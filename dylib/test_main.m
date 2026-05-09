/*
 * test_main.m — XIT1299 dylib integration example (macOS)
 *
 * Build:
 *   make -f Makefile.mac test
 *
 * Run:
 *   DYLD_LIBRARY_PATH=. ./xit1299_test https://yourapp.replit.app/api
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "xit1299.h"

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
        [NSApp activateIgnoringOtherApps:YES];

        const char* api = (argc > 1) ? argv[1] : "http://localhost:80/api";
        NSLog(@"[XIT1299] API: %s", api);

        int result = XIT1299_Activate(api);

        if (result == XIT1299_OK) {
            NSLog(@"[XIT1299] License valid — app continues.");
            /* === YOUR APP CODE HERE === */
        } else {
            NSLog(@"[XIT1299] License failed. Exiting.");
        }

        return result;
    }
}
