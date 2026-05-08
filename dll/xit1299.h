/*
 * XIT1299 License Protection Library
 * Header file for integration
 *
 * Usage:
 *   1. Call XIT1299_Activate() at application startup
 *   2. If it returns XIT1299_SUCCESS, allow the app to continue
 *   3. If it returns XIT1299_FAIL, exit the application
 */

#ifndef XIT1299_H
#define XIT1299_H

#ifdef _WIN32
  #ifdef XIT1299_EXPORTS
    #define XIT1299_API __declspec(dllexport)
  #else
    #define XIT1299_API __declspec(dllimport)
  #endif
#else
  #define XIT1299_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes */
#define XIT1299_SUCCESS  0
#define XIT1299_FAIL     1

/*
 * XIT1299_Activate
 *
 * Shows the @xit1299 license activation dialog.
 * Validates the entered key against the API server.
 * Binds the key to this device (1 key = 1 device only).
 *
 * Parameters:
 *   api_url  - The full base URL of your API server
 *              e.g. "https://yourapp.replit.app/api"
 *
 * Returns:
 *   XIT1299_SUCCESS (0) - Key is valid, app may continue
 *   XIT1299_FAIL    (1) - Key invalid/expired/stolen, exit app
 */
XIT1299_API int XIT1299_Activate(const char* api_url);

/*
 * XIT1299_ValidateKey
 *
 * Validate a key directly without showing the dialog.
 * Useful for background re-validation.
 *
 * Parameters:
 *   api_url   - API base URL
 *   key       - The license key string
 *   device_id - Hardware ID of this device (or NULL to auto-generate)
 *
 * Returns:
 *   XIT1299_SUCCESS (0) - Key is valid
 *   XIT1299_FAIL    (1) - Key is invalid/expired/revoked/device mismatch
 */
XIT1299_API int XIT1299_ValidateKey(const char* api_url, const char* key, const char* device_id);

#ifdef __cplusplus
}
#endif

#endif /* XIT1299_H */
