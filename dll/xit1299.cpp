/*
 * XIT1299 License Protection Library
 * Implementation
 *
 * Build on Windows:
 *   cl /LD /EHsc /DXIT1299_EXPORTS xit1299.cpp /link winhttp.lib user32.lib /OUT:xit1299.dll
 *
 * Build on Linux (for testing):
 *   g++ -shared -fPIC -DXIT1299_EXPORTS -o libxit1299.so xit1299.cpp -lcurl
 *
 * Dependencies:
 *   Windows: WinHTTP (built-in), User32 (built-in)
 *   Linux:   libcurl (apt install libcurl4-openssl-dev)
 */

#include "xit1299.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winhttp.h>
  #include <wincrypt.h>
  #pragma comment(lib, "winhttp.lib")
  #pragma comment(lib, "crypt32.lib")
  #pragma comment(lib, "user32.lib")
#else
  #include <curl/curl.h>
  #include <unistd.h>
  #include <sys/utsname.h>
  #include <stdio.h>
  #include <string.h>
  #include <stdint.h>
#endif

#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdio>

/* ─────────────────────────────────────────────
   Device ID generation
   ───────────────────────────────────────────── */

static std::string get_device_id() {
#ifdef _WIN32
    /* Use machine GUID from registry */
    HKEY hKey;
    char guid[64] = {0};
    DWORD size = sizeof(guid);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, (LPBYTE)guid, &size);
        RegCloseKey(hKey);
    }
    if (strlen(guid) > 0) return std::string(guid);
    /* Fallback: computer name */
    char name[256] = {0};
    DWORD nameLen = sizeof(name);
    GetComputerNameA(name, &nameLen);
    return std::string("WIN-") + name;
#else
    /* Linux: use /etc/machine-id */
    FILE* f = fopen("/etc/machine-id", "r");
    if (f) {
        char id[64] = {0};
        fread(id, 1, sizeof(id)-1, f);
        fclose(f);
        /* strip newline */
        for (int i = 0; id[i]; i++) if (id[i] == '\n') { id[i] = 0; break; }
        if (strlen(id) > 0) return std::string(id);
    }
    /* Fallback: hostname */
    char hostname[256] = {0};
    gethostname(hostname, sizeof(hostname));
    return std::string("LNX-") + hostname;
#endif
}

/* ─────────────────────────────────────────────
   Simple JSON helpers
   ───────────────────────────────────────────── */

static std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

static std::string json_get_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

static bool json_get_bool(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":true";
    return json.find(search) != std::string::npos;
}

/* ─────────────────────────────────────────────
   HTTP POST
   ───────────────────────────────────────────── */

struct HttpResponse {
    int status_code;
    std::string body;
    bool success;
};

#ifdef _WIN32

static HttpResponse http_post(const std::string& url, const std::string& body_json) {
    HttpResponse result = {0, "", false};

    /* Parse URL */
    URL_COMPONENTSA uc = {0};
    uc.dwStructSize      = sizeof(uc);
    char host[256]       = {0};
    char path[1024]      = {0};
    uc.lpszHostName      = host;
    uc.dwHostNameLength  = sizeof(host);
    uc.lpszUrlPath       = path;
    uc.dwUrlPathLength   = sizeof(path);

    if (!WinHttpCrackUrl(std::wstring(url.begin(), url.end()).c_str(), 0, 0, NULL)) {
        /* Try with wide string properly */
        int len = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, NULL, 0);
        std::vector<wchar_t> wurl(len);
        MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wurl.data(), len);

        URL_COMPONENTSW ucw = {0};
        ucw.dwStructSize = sizeof(ucw);
        wchar_t whost[256] = {0};
        wchar_t wpath[1024] = {0};
        ucw.lpszHostName = whost;
        ucw.dwHostNameLength = 256;
        ucw.lpszUrlPath = wpath;
        ucw.dwUrlPathLength = 1024;

        if (!WinHttpCrackUrl(wurl.data(), 0, 0, &ucw)) return result;

        HINTERNET hSession = WinHttpOpen(L"XIT1299/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return result;

        HINTERNET hConnect = WinHttpConnect(hSession, whost, ucw.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

        DWORD flags = (ucw.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath,
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        WinHttpAddRequestHeaders(hRequest,
            L"Content-Type: application/json\r\n", -1L,
            WINHTTP_ADDREQ_FLAG_ADD);

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            (LPVOID)body_json.c_str(), (DWORD)body_json.size(),
            (DWORD)body_json.size(), 0)) {
            if (WinHttpReceiveResponse(hRequest, NULL)) {
                DWORD status = 0;
                DWORD statusSize = sizeof(status);
                WinHttpQueryHeaders(hRequest,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    NULL, &status, &statusSize, NULL);
                result.status_code = (int)status;

                DWORD available = 0;
                while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
                    std::vector<char> buf(available + 1, 0);
                    DWORD read = 0;
                    WinHttpReadData(hRequest, buf.data(), available, &read);
                    result.body += std::string(buf.data(), read);
                }
                result.success = true;
            }
        }
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }
    return result;
}

#else

struct CurlBuffer {
    std::string data;
};

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    CurlBuffer* buf = (CurlBuffer*)userdata;
    buf->data.append(ptr, size * nmemb);
    return size * nmemb;
}

static HttpResponse http_post(const std::string& url, const std::string& body_json) {
    HttpResponse result = {0, "", false};
    CURL* curl = curl_easy_init();
    if (!curl) return result;

    CurlBuffer buf;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_json.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        result.status_code = (int)status;
        result.body = buf.data;
        result.success = true;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

#endif

/* ─────────────────────────────────────────────
   Dialog (Windows) / Console (Linux)
   ───────────────────────────────────────────── */

#ifdef _WIN32

/* Custom dialog: ask for key */
static std::string show_key_input_dialog() {
    /* Use InputBox-style approach with a custom dialog proc */
    /* For simplicity, use a series of MessageBox + a common dialog approach */

    /* We'll use a simple GetSaveFileName trick to get text input, 
       but the cleanest approach is a custom DialogBox. 
       Since we can't embed a resource here, use a CreateWindowEx approach. */

    /* Create a simple window for key input */
    const char* CLASS_NAME = "XIT1299_InputDlg";

    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = DefWindowProcA;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    RegisterClassExA(&wc);

    /* Instead of full custom dialog, use the simplest portable approach:
       An input dialog via VBScript/PowerShell invocation */
    char result_buf[256] = {0};

    /* Use PowerShell to show an input dialog */
    const char* ps_cmd =
        "powershell -Command \""
        "$key = [Microsoft.VisualBasic.Interaction]::InputBox("
        "'Enter your @xit1299 key to continue.', '@xit1299', '');"
        "[System.IO.File]::WriteAllText($env:TEMP + '\\xit_key.tmp', $key)"
        "\"";

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s", ps_cmd);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        /* Fallback: simple message box loop */
        MessageBoxA(NULL,
            "Could not show input dialog.\nPlease check your PowerShell installation.",
            "@xit1299", MB_OK | MB_ICONERROR);
        return "";
    }
    WaitForSingleObject(pi.hProcess, 30000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /* Read the temp file */
    char tmp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp_path);
    std::string file_path = std::string(tmp_path) + "xit_key.tmp";

    FILE* f = fopen(file_path.c_str(), "r");
    if (f) {
        fgets(result_buf, sizeof(result_buf), f);
        fclose(f);
        DeleteFileA(file_path.c_str());
        /* Trim whitespace */
        int len = strlen(result_buf);
        while (len > 0 && (result_buf[len-1] == '\n' || result_buf[len-1] == '\r' || result_buf[len-1] == ' '))
            result_buf[--len] = 0;
    }
    return std::string(result_buf);
}

static void show_success_dialog(const std::string& time_remaining) {
    std::string msg = "@xit1299 VIP activated\n" +
                      time_remaining + "\n\n" +
                      "Protected by @xit1299";
    MessageBoxA(NULL, msg.c_str(), "@xit1299", MB_OK | MB_ICONINFORMATION);
}

static void show_error_dialog(const std::string& error_msg) {
    MessageBoxA(NULL, error_msg.c_str(), "@xit1299", MB_OK | MB_ICONERROR);
}

static bool show_quit_confirm() {
    int ret = MessageBoxA(NULL,
        "Enter your @xit1299 key to continue.\n\nPress OK to enter key, Cancel to quit.",
        "@xit1299", MB_OKCANCEL | MB_ICONQUESTION);
    return (ret == IDCANCEL);
}

#else /* Linux/Mac — console fallback */

static std::string show_key_input_dialog() {
    char buf[256] = {0};
    printf("\n╔══════════════════════════════════╗\n");
    printf("║           @xit1299               ║\n");
    printf("║  Enter your @xit1299 key         ║\n");
    printf("║  to continue.                    ║\n");
    printf("╚══════════════════════════════════╝\n");
    printf("Paste license key: ");
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin)) {
        int len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
            buf[--len] = 0;
    }
    return std::string(buf);
}

static void show_success_dialog(const std::string& time_remaining) {
    printf("\n╔══════════════════════════════════╗\n");
    printf("║           @xit1299               ║\n");
    printf("║  [OK] @xit1299 VIP activated     ║\n");
    printf("║  [*] %-28s║\n", time_remaining.c_str());
    printf("║                                  ║\n");
    printf("║  [=] Protected by @xit1299       ║\n");
    printf("╚══════════════════════════════════╝\n");
    printf("\n  Enter App\n\n");
    fflush(stdout);
}

static void show_error_dialog(const std::string& error_msg) {
    fprintf(stderr, "\n[@xit1299] ERROR: %s\n", error_msg.c_str());
}

static bool show_quit_confirm() {
    printf("\nPress Enter to enter key, or type 'quit' to exit: ");
    fflush(stdout);
    char buf[16] = {0};
    fgets(buf, sizeof(buf), stdin);
    return (strncmp(buf, "quit", 4) == 0);
}

#endif

/* ─────────────────────────────────────────────
   Core logic
   ───────────────────────────────────────────── */

static int validate_key_api(const std::string& api_url, const std::string& key,
                             const std::string& device_id, std::string& out_msg,
                             std::string& out_time) {
    /* Build JSON body */
    std::string body = "{\"key\":\"" + json_escape(key) + "\","
                       "\"deviceId\":\"" + json_escape(device_id) + "\"}";

    std::string endpoint = api_url;
    if (!endpoint.empty() && endpoint.back() == '/') endpoint.pop_back();
    endpoint += "/keys/validate";

    HttpResponse resp = http_post(endpoint, body);

    if (!resp.success || resp.status_code != 200) {
        out_msg = "Cannot connect to @xit1299 server. Check your internet connection.";
        return XIT1299_FAIL;
    }

    bool valid       = json_get_bool(resp.body, "valid");
    out_msg          = json_get_string(resp.body, "message");
    out_time         = json_get_string(resp.body, "timeRemaining");

    return valid ? XIT1299_SUCCESS : XIT1299_FAIL;
}

/* ─────────────────────────────────────────────
   Public API
   ───────────────────────────────────────────── */

extern "C" XIT1299_API int XIT1299_ValidateKey(const char* api_url, const char* key,
                                                const char* device_id) {
    if (!api_url || !key) return XIT1299_FAIL;

    std::string dev_id = (device_id && strlen(device_id) > 0)
                         ? std::string(device_id)
                         : get_device_id();

    std::string msg, time_remaining;
    return validate_key_api(std::string(api_url), std::string(key), dev_id, msg, time_remaining);
}

extern "C" XIT1299_API int XIT1299_Activate(const char* api_url) {
    if (!api_url) return XIT1299_FAIL;

    std::string device_id = get_device_id();
    std::string base_url  = std::string(api_url);

    /* Show initial prompt */
    if (show_quit_confirm()) {
        /* User chose to quit */
        return XIT1299_FAIL;
    }

    /* Retry loop — user gets 3 attempts */
    for (int attempt = 0; attempt < 3; ++attempt) {
        std::string key = show_key_input_dialog();
        if (key.empty()) {
            /* User cancelled */
            return XIT1299_FAIL;
        }

        std::string msg, time_remaining;
        int result = validate_key_api(base_url, key, device_id, msg, time_remaining);

        if (result == XIT1299_SUCCESS) {
            /* Compute display remaining time */
            std::string display_time = time_remaining.empty() ? "Key active" : time_remaining;
            show_success_dialog(display_time);
            return XIT1299_SUCCESS;
        } else {
            if (attempt < 2) {
                show_error_dialog(msg + "\n\nPlease try again.");
            } else {
                show_error_dialog(msg + "\n\nNo more attempts. Exiting.");
            }
        }
    }

    return XIT1299_FAIL;
}
