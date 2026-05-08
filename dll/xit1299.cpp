/*
 * xit1299.cpp — XIT1299 License Protection Library
 *
 * Windows: Custom Win32 dialogs (dark theme, @xit1299 branding)
 * Linux  : Console fallback for testing
 *
 * Build on Windows (MSVC + CMake):
 *   mkdir build && cd build
 *   cmake .. -G "Visual Studio 17 2022"
 *   cmake --build . --config Release
 *   -> Release/xit1299.dll + Release/xit1299.lib
 *
 * Build on Linux (GCC + CMake):
 *   mkdir build && cd build
 *   cmake ..
 *   make
 *   -> libxit1299.so
 */

#include "xit1299.h"

/* ═══════════════════════════════════════════════════════════════
   Platform headers
   ═══════════════════════════════════════════════════════════════ */
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <winhttp.h>
  #include <commctrl.h>
  #pragma comment(lib, "winhttp.lib")
  #pragma comment(lib, "comctl32.lib")
  #pragma comment(lib, "user32.lib")
  #pragma comment(lib, "gdi32.lib")
  #pragma comment(lib, "advapi32.lib")
#else
  #include <curl/curl.h>
  #include <sys/utsname.h>
  #include <unistd.h>
  #include <termios.h>
#endif

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

/* ═══════════════════════════════════════════════════════════════
   Colours & layout constants (dark command-center theme)
   ═══════════════════════════════════════════════════════════════ */
#ifdef _WIN32
static const COLORREF COL_BG        = RGB(13, 13, 13);   /* near-black bg          */
static const COLORREF COL_CARD      = RGB(22, 22, 22);   /* card surface           */
static const COLORREF COL_ACCENT    = RGB(0, 200, 220);  /* cyan @xit1299 accent   */
static const COLORREF COL_TEXT      = RGB(220, 220, 220);/* primary text           */
static const COLORREF COL_SUBTEXT   = RGB(120, 120, 130);/* muted text             */
static const COLORREF COL_BORDER    = RGB(40, 40, 50);   /* border                 */
static const COLORREF COL_BTN_PRI   = RGB(0, 200, 220);  /* primary button fill    */
static const COLORREF COL_BTN_SEC   = RGB(35, 35, 45);   /* secondary button fill  */
static const COLORREF COL_INPUT_BG  = RGB(18, 18, 28);   /* input field background */
static const COLORREF COL_SUCCESS   = RGB(0, 210, 130);  /* green success          */
#endif

/* ═══════════════════════════════════════════════════════════════
   JSON helpers (no external deps)
   ═══════════════════════════════════════════════════════════════ */
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;
        }
    }
    return out;
}

static std::string json_str(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    size_t p = json.find(needle);
    if (p == std::string::npos) return "";
    p += needle.size();
    size_t e = json.find('"', p);
    return (e == std::string::npos) ? "" : json.substr(p, e - p);
}

static bool json_bool(const std::string& json, const std::string& key) {
    return json.find("\"" + key + "\":true") != std::string::npos;
}

/* ═══════════════════════════════════════════════════════════════
   Device ID
   ═══════════════════════════════════════════════════════════════ */
static std::string get_device_id() {
#ifdef _WIN32
    /* Read MachineGuid from registry (unique per Windows install) */
    HKEY hk;
    LONG r = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography", 0,
        KEY_READ | KEY_WOW64_64KEY, &hk);
    if (r == ERROR_SUCCESS) {
        char buf[64] = {0};
        DWORD sz = sizeof(buf);
        RegQueryValueExA(hk, "MachineGuid", NULL, NULL, (LPBYTE)buf, &sz);
        RegCloseKey(hk);
        if (buf[0]) return std::string("WIN:") + buf;
    }
    /* Fallback: computer name */
    char name[256] = {0};
    DWORD len = sizeof(name);
    GetComputerNameA(name, &len);
    return std::string("WIN-NAME:") + name;
#else
    /* Linux: /etc/machine-id */
    FILE* f = fopen("/etc/machine-id", "r");
    if (f) {
        char id[64] = {0};
        if (fgets(id, sizeof(id), f)) {
            fclose(f);
            /* strip newline */
            for (char* p = id; *p; p++) if (*p == '\n') { *p = 0; break; }
            if (id[0]) return std::string("LNX:") + id;
        } else { fclose(f); }
    }
    char host[256] = {0};
    gethostname(host, sizeof(host));
    return std::string("LNX-HOST:") + host;
#endif
}

/* ═══════════════════════════════════════════════════════════════
   HTTP POST
   ═══════════════════════════════════════════════════════════════ */
struct HttpResult {
    bool    ok;
    int     status;
    std::string body;
};

#ifdef _WIN32

/* Convert narrow to wide string */
static std::wstring to_wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    std::vector<wchar_t> buf(n);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buf.data(), n);
    return std::wstring(buf.data());
}

static HttpResult http_post(const std::string& url, const std::string& body) {
    HttpResult res = {false, 0, ""};

    std::wstring wurl = to_wide(url);

    URL_COMPONENTSW uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t whost[256] = {};
    wchar_t wpath[1024] = {};
    uc.lpszHostName     = whost; uc.dwHostNameLength  = 256;
    uc.lpszUrlPath      = wpath; uc.dwUrlPathLength   = 1024;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return res;

    HINTERNET hSess = WinHttpOpen(L"xit1299/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return res;

    HINTERNET hConn = WinHttpConnect(hSess, whost, uc.nPort, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return res; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", wpath,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return res; }

    WinHttpAddRequestHeaders(hReq,
        L"Content-Type: application/json\r\n", (DWORD)-1,
        WINHTTP_ADDREQ_FLAG_ADD);

    BOOL sent = WinHttpSendRequest(hReq,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)body.c_str(), (DWORD)body.size(),
        (DWORD)body.size(), 0);

    if (sent && WinHttpReceiveResponse(hReq, NULL)) {
        DWORD status = 0; DWORD sz = sizeof(status);
        WinHttpQueryHeaders(hReq,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &status, &sz, NULL);
        res.status = (int)status;

        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(hReq, &avail) && avail > 0) {
            std::vector<char> buf(avail + 1, 0);
            DWORD rd = 0;
            WinHttpReadData(hReq, buf.data(), avail, &rd);
            res.body.append(buf.data(), rd);
        }
        res.ok = true;
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return res;
}

#else /* Linux — libcurl */

struct CurlBuf { std::string data; };
static size_t curl_cb(char* p, size_t sz, size_t n, void* u) {
    ((CurlBuf*)u)->data.append(p, sz * n);
    return sz * n;
}

static HttpResult http_post(const std::string& url, const std::string& body) {
    HttpResult res = {false, 0, ""};
    CURL* c = curl_easy_init();
    if (!c) return res;

    CurlBuf buf;
    curl_slist* hdrs = curl_slist_append(NULL, "Content-Type: application/json");
    curl_easy_setopt(c, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(c, CURLOPT_POST,           1L);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE,  (long)body.size());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER,     hdrs);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION,  curl_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA,      &buf);
    curl_easy_setopt(c, CURLOPT_TIMEOUT,        10L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);

    if (curl_easy_perform(c) == CURLE_OK) {
        long st = 0;
        curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &st);
        res.status = (int)st;
        res.body   = buf.data;
        res.ok     = true;
    }
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return res;
}
#endif /* HTTP */

/* ═══════════════════════════════════════════════════════════════
   Core validate call
   ═══════════════════════════════════════════════════════════════ */
struct ValidateResult {
    bool        valid;
    bool        device_locked;
    std::string message;
    std::string time_remaining;
};

static ValidateResult call_validate(const std::string& api_base,
                                    const std::string& key,
                                    const std::string& device_id) {
    ValidateResult r = {false, false, "Cannot reach @xit1299 server.", ""};

    std::string base = api_base;
    while (!base.empty() && base.back() == '/') base.pop_back();
    std::string url = base + "/keys/validate";

    std::string body = "{\"key\":\"" + json_escape(key) +
                       "\",\"deviceId\":\"" + json_escape(device_id) + "\"}";

    HttpResult hr = http_post(url, body);
    if (!hr.ok || hr.status != 200) {
        if (hr.status == 0)
            r.message = "Cannot connect to @xit1299 server. Check your internet.";
        else
            r.message = "Server error (HTTP " + std::to_string(hr.status) + ")";
        return r;
    }

    r.valid          = json_bool(hr.body, "valid");
    r.device_locked  = json_bool(hr.body, "deviceLocked");
    r.message        = json_str(hr.body,  "message");
    r.time_remaining = json_str(hr.body,  "timeRemaining");
    return r;
}

/* ═══════════════════════════════════════════════════════════════
   WIN32 — Custom dialogs
   ═══════════════════════════════════════════════════════════════ */
#ifdef _WIN32

/* ── Shared GDI resources ── */
static HFONT g_fntTitle   = NULL;
static HFONT g_fntBody    = NULL;
static HFONT g_fntMono    = NULL;
static HBRUSH g_brBg      = NULL;
static HBRUSH g_brCard    = NULL;
static HBRUSH g_brInput   = NULL;
static HBRUSH g_brBtnPri  = NULL;
static HBRUSH g_brBtnSec  = NULL;

static void init_resources() {
    if (g_fntTitle) return;
    g_fntTitle  = CreateFontA(22, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH, "Segoe UI");
    g_fntBody   = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH, "Segoe UI");
    g_fntMono   = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH, "Consolas");
    g_brBg      = CreateSolidBrush(COL_BG);
    g_brCard    = CreateSolidBrush(COL_CARD);
    g_brInput   = CreateSolidBrush(COL_INPUT_BG);
    g_brBtnPri  = CreateSolidBrush(COL_BTN_PRI);
    g_brBtnSec  = CreateSolidBrush(COL_BTN_SEC);
}

/* Draw rounded rectangle helper */
static void fill_rounded(HDC hdc, RECT r, int radius, COLORREF col) {
    HBRUSH br = CreateSolidBrush(col);
    HPEN   pn = CreatePen(PS_NULL, 0, col);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
    DeleteObject(pn);
}

/* ─────────────────────────────────────────────────────────────
   DIALOG 1 — Activation (Photo 1)
   ───────────────────────────────────────────────────────────── */
#define DLG1_W  420
#define DLG1_H  310

#define ID_EDIT_KEY   101
#define ID_BTN_ACT    102
#define ID_BTN_QUIT   103
#define ID_LBL_ERR    104

struct DLG1_STATE {
    std::string result_key;   /* output: key entered by user */
    bool        activated;    /* true if user clicked Activate */
    char        err_msg[256]; /* error text shown under input  */
    HWND        hEdit;
    HWND        hBtnAct;
    HWND        hBtnQuit;
    HWND        hLblErr;
};

static LRESULT CALLBACK Dlg1WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    DLG1_STATE* st = (DLG1_STATE*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        init_resources();
        DLG1_STATE* s = (DLG1_STATE*)((CREATESTRUCTA*)lp)->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)s);

        /* ---- Edit box ---- */
        s->hEdit = CreateWindowExA(0, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            30, 155, 360, 34, hwnd, (HMENU)ID_EDIT_KEY,
            GetModuleHandleA(NULL), NULL);
        SendMessageA(s->hEdit, EM_SETLIMITTEXT, 50, 0);
        SendMessageA(s->hEdit, WM_SETFONT, (WPARAM)g_fntMono, TRUE);

        /* placeholder — "Paste license key" */
        SetWindowTextA(s->hEdit, "Paste license key");

        /* ---- Error label ---- */
        s->hLblErr = CreateWindowExA(0, "STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            30, 194, 360, 18, hwnd, (HMENU)ID_LBL_ERR,
            GetModuleHandleA(NULL), NULL);
        SendMessageA(s->hLblErr, WM_SETFONT, (WPARAM)g_fntBody, TRUE);

        /* ---- Buttons ---- */
        s->hBtnAct = CreateWindowExA(0, "BUTTON", "Activate",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
            30, 228, 170, 38, hwnd, (HMENU)ID_BTN_ACT,
            GetModuleHandleA(NULL), NULL);
        SendMessageA(s->hBtnAct, WM_SETFONT, (WPARAM)g_fntBody, TRUE);

        s->hBtnQuit = CreateWindowExA(0, "BUTTON", "Quit",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
            220, 228, 170, 38, hwnd, (HMENU)ID_BTN_QUIT,
            GetModuleHandleA(NULL), NULL);
        SendMessageA(s->hBtnQuit, WM_SETFONT, (WPARAM)g_fntBody, TRUE);

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc; GetClientRect(hwnd, &rc);

        /* Background */
        FillRect(hdc, &rc, g_brBg);

        /* Header accent stripe */
        RECT hdr = {0, 0, DLG1_W, 8};
        FillRect(hdc, &hdr, g_brBtnPri);

        /* Card area */
        RECT card = {20, 20, DLG1_W - 20, DLG1_H - 20};
        fill_rounded(hdc, card, 10, COL_CARD);

        /* Title — @xit1299 */
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COL_ACCENT);
        SelectObject(hdc, g_fntTitle);
        RECT trTitle = {30, 30, DLG1_W - 30, 60};
        DrawTextA(hdc, "@xit1299", -1, &trTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        /* Separator */
        HPEN pen = CreatePen(PS_SOLID, 1, COL_BORDER);
        HPEN op  = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 30, 65, NULL);
        LineTo(hdc, DLG1_W - 30, 65);
        SelectObject(hdc, op);
        DeleteObject(pen);

        /* Subtitle */
        SetTextColor(hdc, COL_TEXT);
        SelectObject(hdc, g_fntBody);
        RECT trSub = {30, 75, DLG1_W - 30, 110};
        DrawTextA(hdc, "Enter your @xit1299 key to continue.", -1, &trSub,
                  DT_LEFT | DT_WORDBREAK);

        /* Input label */
        SetTextColor(hdc, COL_SUBTEXT);
        RECT trLbl = {30, 128, 200, 150};
        DrawTextA(hdc, "LICENSE KEY", -1, &trLbl, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        EndPaint(hwnd, &ps);
        return 0;
    }

    /* Color the edit box */
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, COL_INPUT_BG);
        SetTextColor(hdc, COL_TEXT);
        return (LRESULT)g_brInput;
    }

    /* Color static labels */
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        HWND hctl = (HWND)lp;
        SetBkMode(hdc, TRANSPARENT);
        if (st && hctl == st->hLblErr)
            SetTextColor(hdc, RGB(255, 80, 80));
        else
            SetTextColor(hdc, COL_TEXT);
        return (LRESULT)g_brBg;
    }

    /* Owner-draw buttons */
    case WM_DRAWITEM: {
        if (!st) return 0;
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lp;
        HDC hdc = di->hDC;
        RECT r  = di->rcItem;

        bool is_primary = (di->CtlID == ID_BTN_ACT);
        COLORREF col_fill = is_primary ? COL_BTN_PRI : COL_BTN_SEC;
        COLORREF col_text = is_primary ? COL_BG      : COL_TEXT;

        if (di->itemState & ODS_SELECTED) {
            col_fill = is_primary ? RGB(0, 160, 175) : RGB(50, 50, 65);
        }

        fill_rounded(hdc, r, 6, col_fill);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, col_text);
        SelectObject(hdc, g_fntBody);
        char buf[64]; GetWindowTextA(di->hwndItem, buf, sizeof(buf));
        DrawTextA(hdc, buf, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    case WM_COMMAND: {
        if (!st) return 0;
        WORD id = LOWORD(wp);
        if (id == ID_BTN_ACT) {
            char buf[64] = {0};
            GetWindowTextA(st->hEdit, buf, sizeof(buf));
            /* ignore placeholder */
            if (buf[0] && strcmp(buf, "Paste license key") != 0) {
                st->result_key = buf;
                st->activated  = true;
                DestroyWindow(hwnd);
            } else {
                SetWindowTextA(st->hLblErr, "Please paste your license key first.");
            }
        } else if (id == ID_BTN_QUIT) {
            st->activated = false;
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_SETCURSOR:
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 1; /* prevent flicker */
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static DLG1_STATE show_activate_dialog() {
    DLG1_STATE st = {};
    st.activated = false;

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXA wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = Dlg1WndProc;
        wc.hInstance     = GetModuleHandleA(NULL);
        wc.lpszClassName = "XIT1299_DLG1";
        wc.hbrBackground = NULL;
        wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExA(&wc);
        registered = true;
    }

    /* Center on screen */
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    int x  = (sx - DLG1_W) / 2;
    int y  = (sy - DLG1_H) / 2;

    HWND hw = CreateWindowExA(
        WS_EX_APPWINDOW | WS_EX_TOPMOST,
        "XIT1299_DLG1", "@xit1299",
        WS_POPUP | WS_VISIBLE | WS_THICKFRAME,
        x, y, DLG1_W, DLG1_H,
        NULL, NULL, GetModuleHandleA(NULL), &st);

    SetForegroundWindow(hw);
    SetFocus(hw);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return st;
}

/* ─────────────────────────────────────────────────────────────
   DIALOG 2 — Success (Photo 2)
   ───────────────────────────────────────────────────────────── */
#define DLG2_W  380
#define DLG2_H  280
#define ID_BTN_ENTER  201

struct DLG2_STATE {
    std::string time_remaining;
};

static LRESULT CALLBACK Dlg2WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    DLG2_STATE* st = (DLG2_STATE*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        init_resources();
        DLG2_STATE* s = (DLG2_STATE*)((CREATESTRUCTA*)lp)->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)s);

        HWND hBtn = CreateWindowExA(0, "BUTTON", "Enter App",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_OWNERDRAW,
            (DLG2_W - 160) / 2, DLG2_H - 60, 160, 38,
            hwnd, (HMENU)ID_BTN_ENTER,
            GetModuleHandleA(NULL), NULL);
        SendMessageA(hBtn, WM_SETFONT, (WPARAM)g_fntBody, TRUE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        /* Background */
        FillRect(hdc, &rc, g_brBg);

        /* Header stripe */
        RECT hdr = {0, 0, DLG2_W, 8};
        FillRect(hdc, &hdr, g_brBtnPri);

        /* Card */
        RECT card = {16, 16, DLG2_W - 16, DLG2_H - 16};
        fill_rounded(hdc, card, 10, COL_CARD);

        /* Title */
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COL_ACCENT);
        SelectObject(hdc, g_fntTitle);
        RECT tTitle = {24, 26, DLG2_W - 24, 54};
        DrawTextA(hdc, "@xit1299", -1, &tTitle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        /* Separator */
        HPEN pen = CreatePen(PS_SOLID, 1, COL_BORDER);
        HPEN op  = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 24, 60, NULL);
        LineTo(hdc, DLG2_W - 24, 60);
        SelectObject(hdc, op);
        DeleteObject(pen);

        SelectObject(hdc, g_fntBody);

        /* Row 1: VIP activated */
        SetTextColor(hdc, COL_SUCCESS);
        RECT r1 = {34, 76, DLG2_W - 34, 98};
        DrawTextA(hdc, "\xE2\x9C\x94  @xit1299 VIP activated", -1, &r1,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        /* Row 2: time remaining */
        std::string tr_line = "\xE2\x8F\xB3  ";
        if (st) tr_line += st->time_remaining;
        SetTextColor(hdc, COL_TEXT);
        RECT r2 = {34, 108, DLG2_W - 34, 130};
        DrawTextA(hdc, tr_line.c_str(), -1, &r2,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        /* Row 3: Protected */
        SetTextColor(hdc, COL_SUBTEXT);
        RECT r3 = {34, 140, DLG2_W - 34, 162};
        DrawTextA(hdc, "\xF0\x9F\x94\x92  Protected by @xit1299", -1, &r3,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COL_TEXT);
        return (LRESULT)g_brBg;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lp;
        HDC hdc = di->hDC;
        RECT r  = di->rcItem;
        COLORREF fill = (di->itemState & ODS_SELECTED)
            ? RGB(0, 160, 175) : COL_BTN_PRI;
        fill_rounded(hdc, r, 6, fill);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COL_BG);
        SelectObject(hdc, g_fntBody);
        DrawTextA(hdc, "Enter App", -1, &r,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == ID_BTN_ENTER) DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void show_success_dialog(const std::string& time_remaining) {
    DLG2_STATE st;
    st.time_remaining = time_remaining.empty() ? "Key is active" : time_remaining;

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXA wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = Dlg2WndProc;
        wc.hInstance     = GetModuleHandleA(NULL);
        wc.lpszClassName = "XIT1299_DLG2";
        wc.hbrBackground = NULL;
        wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExA(&wc);
        registered = true;
    }

    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    HWND hw = CreateWindowExA(
        WS_EX_APPWINDOW | WS_EX_TOPMOST,
        "XIT1299_DLG2", "@xit1299",
        WS_POPUP | WS_VISIBLE | WS_THICKFRAME,
        (sx - DLG2_W) / 2, (sy - DLG2_H) / 2,
        DLG2_W, DLG2_H,
        NULL, NULL, GetModuleHandleA(NULL), &st);

    SetForegroundWindow(hw);
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static void show_error_msg(const std::string& text) {
    MessageBoxA(NULL, text.c_str(), "@xit1299 — Error",
                MB_OK | MB_ICONERROR | MB_TOPMOST);
}

#else /* ────────── Linux / Console fallback ────────── */

static std::string prompt_key() {
    printf("\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║            @xit1299                  ║\n");
    printf("  ║  Enter your @xit1299 key to          ║\n");
    printf("  ║  continue.                           ║\n");
    printf("  ╠══════════════════════════════════════╣\n");
    printf("  ║  Paste license key:                  ║\n");
    printf("  ╚══════════════════════════════════════╝\n");
    printf("  > ");
    fflush(stdout);

    char buf[128] = {0};
    if (!fgets(buf, sizeof(buf), stdin)) return "";
    for (char* p = buf; *p; p++) if (*p == '\n' || *p == '\r') { *p = 0; break; }
    return std::string(buf);
}

static void show_success_dialog(const std::string& time_remaining) {
    std::string tr = time_remaining.empty() ? "Key active" : time_remaining;
    printf("\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║            @xit1299                  ║\n");
    printf("  ╠══════════════════════════════════════╣\n");
    printf("  ║  [OK] @xit1299 VIP activated         ║\n");
    printf("  ║  [*]  %-33s║\n", tr.c_str());
    printf("  ║                                      ║\n");
    printf("  ║  [=]  Protected by @xit1299          ║\n");
    printf("  ╠══════════════════════════════════════╣\n");
    printf("  ║             Enter App >              ║\n");
    printf("  ╚══════════════════════════════════════╝\n\n");
    fflush(stdout);
}

static void show_error_msg(const std::string& text) {
    fprintf(stderr, "\n  [@xit1299] ERROR: %s\n\n", text.c_str());
}

#endif /* platform */

/* ═══════════════════════════════════════════════════════════════
   Public API implementations
   ═══════════════════════════════════════════════════════════════ */

extern "C" XIT1299_API int XIT1299_CALL
XIT1299_ValidateSilent(const char* api_base_url,
                       const char* key,
                       const char* device_id) {
    if (!api_base_url || !key) return XIT1299_FAIL;
    std::string dev = (device_id && device_id[0]) ? device_id : get_device_id();
    ValidateResult r = call_validate(api_base_url, key, dev);
    return r.valid ? XIT1299_OK : XIT1299_FAIL;
}

extern "C" XIT1299_API int XIT1299_CALL
XIT1299_Activate(const char* api_base_url) {
    if (!api_base_url) return XIT1299_FAIL;

    std::string base      = api_base_url;
    std::string device_id = get_device_id();

    const int MAX_ATTEMPTS = 3;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {

#ifdef _WIN32
        DLG1_STATE dlg = show_activate_dialog();
        if (!dlg.activated) return XIT1299_FAIL; /* user pressed Quit */
        std::string key = dlg.result_key;
#else
        std::string key = prompt_key();
        if (key.empty()) return XIT1299_FAIL;
#endif

        ValidateResult vr = call_validate(base, key, device_id);

        if (vr.valid) {
            show_success_dialog(vr.time_remaining);
            return XIT1299_OK;
        }

        std::string err = vr.message;
        if (err.empty()) err = "License key is not valid.";

        if (attempt < MAX_ATTEMPTS) {
            err += "\n\nAttempt " + std::to_string(attempt) + " of " +
                   std::to_string(MAX_ATTEMPTS) + " — please try again.";
        } else {
            err += "\n\nNo attempts remaining. The application will now exit.";
        }
        show_error_msg(err);
    }

    return XIT1299_FAIL;
}
