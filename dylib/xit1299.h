/*
 * xit1299.h — XIT1299 License Protection (macOS / iOS dylib)
 *
 * Build macOS:
 *   make -f Makefile.mac
 *   -> xit1299.dylib
 *
 * Build iOS (Theos / jailbreak):
 *   make -f Makefile.theos
 *   -> xit1299.dylib
 */

#ifndef XIT1299_H
#define XIT1299_H

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes */
#define XIT1299_OK    0
#define XIT1299_FAIL  1

/*
 * XIT1299_Activate
 *
 * Shows the @xit1299 activation dialog.
 * Validates key against API. Binds key to this device (1 key = 1 device).
 *
 * api_base_url — e.g. "https://yourapp.replit.app/api"
 *
 * Returns XIT1299_OK (0) if valid, XIT1299_FAIL (1) otherwise.
 */
int XIT1299_Activate(const char* api_base_url);

/*
 * XIT1299_ValidateSilent
 *
 * Validate without UI (background re-check).
 * Pass NULL for device_id to auto-detect.
 */
int XIT1299_ValidateSilent(const char* api_base_url,
                           const char* key,
                           const char* device_id);

#ifdef __cplusplus
}
#endif

#endif /* XIT1299_H */
