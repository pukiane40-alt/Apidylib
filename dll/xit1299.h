/*
 * xit1299.h — Public API for the XIT1299 License Protection Library
 *
 * Include this header in your application, link against xit1299.lib (Windows)
 * or libxit1299.so (Linux), then call XIT1299_Activate() at startup.
 *
 * Build:
 *   Windows : cmake .. -G "Visual Studio 17 2022" && cmake --build . --config Release
 *   Linux   : cmake .. && make
 */

#ifndef XIT1299_H
#define XIT1299_H

#ifdef _WIN32
  #ifdef XIT1299_EXPORTS
    #define XIT1299_API __declspec(dllexport)
  #else
    #define XIT1299_API __declspec(dllimport)
  #endif
  #define XIT1299_CALL __cdecl
#else
  #define XIT1299_API  __attribute__((visibility("default")))
  #define XIT1299_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes */
#define XIT1299_OK    0   /* Key valid — allow app to run  */
#define XIT1299_FAIL  1   /* Key invalid / user quit       */

/*
 * XIT1299_Activate
 *
 * Show the @xit1299 activation dialog, validate the key against the API
 * server, and bind the key to this device (1 key = 1 device only).
 *
 * Parameters:
 *   api_base_url — e.g. "https://yourapp.replit.app/api"
 *
 * Returns XIT1299_OK on success, XIT1299_FAIL otherwise.
 * Call at the very beginning of main()/WinMain().
 */
XIT1299_API int XIT1299_CALL XIT1299_Activate(const char* api_base_url);

/*
 * XIT1299_ValidateSilent
 *
 * Validate a key without any UI (background re-check).
 *
 * Parameters:
 *   api_base_url — API base URL
 *   key          — License key string (XIT-XXXXXXXX-XXXXXXXX-XXXXXXXX)
 *   device_id    — Hardware ID (pass NULL to auto-detect)
 *
 * Returns XIT1299_OK if valid, XIT1299_FAIL otherwise.
 */
XIT1299_API int XIT1299_CALL XIT1299_ValidateSilent(const char* api_base_url,
                                                     const char* key,
                                                     const char* device_id);

#ifdef __cplusplus
}
#endif

#endif /* XIT1299_H */
