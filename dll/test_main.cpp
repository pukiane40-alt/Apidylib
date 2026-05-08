/*
 * XIT1299 DLL - Integration Test / Example
 *
 * This shows how to integrate the DLL into your application.
 *
 * Build with CMake:
 *   mkdir build && cd build
 *   cmake ..
 *   cmake --build . --config Release
 *
 * Then run:
 *   ./xit1299_test https://yourapp.replit.app/api
 */

#include <stdio.h>
#include <stdlib.h>
#include "xit1299.h"

int main(int argc, char* argv[]) {
    const char* api_url = (argc > 1) ? argv[1] : "http://localhost:80/api";

    printf("XIT1299 License Check\n");
    printf("API: %s\n\n", api_url);

    /*
     * Call XIT1299_Activate() at your application startup.
     * It shows the dialog, asks for a key, validates it against the API,
     * and binds it to this device.
     *
     * If it returns XIT1299_FAIL, you must exit.
     */
    int result = XIT1299_Activate(api_url);

    if (result == XIT1299_SUCCESS) {
        printf("License OK — entering application.\n");
        /* === YOUR APPLICATION STARTS HERE === */
        /* ... your code ... */
        return 0;
    } else {
        printf("License check failed. Exiting.\n");
        return 1;
    }
}
