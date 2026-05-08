/*
 * test_main.cpp — Example integration + smoke test
 *
 * Usage:
 *   xit1299_test <api_base_url>
 *
 * Example:
 *   xit1299_test https://yourapp.replit.app/api
 *
 * Build (CMake):
 *   mkdir build && cd build
 *   cmake .. -DCMAKE_BUILD_TYPE=Release
 *   cmake --build .
 *   ./xit1299_test https://yourapp.replit.app/api
 */

#include "xit1299.h"
#include <cstdio>
#include <cstring>

int main(int argc, char* argv[]) {
    const char* api = (argc > 1) ? argv[1] : "http://localhost:80/api";

    printf("=== XIT1299 License Check ===\n");
    printf("API endpoint : %s\n\n", api);

    /*
     * XIT1299_Activate() does everything:
     *   1. Shows the "@xit1299" activation dialog
     *   2. Validates the key against the API server
     *   3. Binds the key to this machine (1 key = 1 device)
     *   4. Shows the success dialog with time remaining
     *
     * Returns XIT1299_OK  (0) → key valid, app may continue
     *         XIT1299_FAIL(1) → key invalid / user quit → exit
     */
    int result = XIT1299_Activate(api);

    if (result == XIT1299_OK) {
        printf("[XIT1299] License valid — entering application.\n");
        /* === YOUR APPLICATION CODE STARTS HERE === */
        /* ... */
        return 0;
    }

    printf("[XIT1299] License check failed. Exiting.\n");
    return 1;
}
