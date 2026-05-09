# XIT1299 dylib — macOS / iOS License Protection

Protect your macOS app or iOS jailbreak tweak with @xit1299 license keys.
**1 key = 1 device only** — bound permanently on first activation.

---

## Files

| File | Purpose |
|------|---------|
| `xit1299.h` | Public header — include in your project |
| `xit1299.m` | Objective-C implementation |
| `Makefile.mac` | Build for macOS (AppKit dialogs) |
| `Makefile.theos` | Build for iOS jailbreak (Theos + UIKit) |
| `test_main.m` | macOS test / example |

---

## Build — macOS

Requirements: **Xcode** or **Xcode Command Line Tools**

```bash
# Install command line tools if needed
xcode-select --install

# Build the dylib
cd dylib
make -f Makefile.mac

# Output: xit1299.dylib
```

### Add to your Xcode project

1. Drag `xit1299.dylib` and `xit1299.h` into your Xcode project
2. In **Build Phases → Link Binary With Libraries** → add `xit1299.dylib`
3. Set **Runpath Search Paths** to `@executable_path`

### Use in code (Objective-C)

```objc
#import "xit1299.h"

- (void)applicationDidFinishLaunching:(NSNotification*)n {
    if (XIT1299_Activate("https://yourapp.replit.app/api") != XIT1299_OK) {
        [NSApp terminate:nil]; // exit if invalid
    }
    // app continues here
}
```

### Use in code (Swift)

```swift
// Add a bridging header with: #include "xit1299.h"
func applicationDidFinishLaunching(_ n: Notification) {
    if XIT1299_Activate("https://yourapp.replit.app/api") != XIT1299_OK {
        NSApp.terminate(nil)
    }
}
```

---

## Build — iOS Jailbreak (Theos)

Requirements: [Theos](https://theos.dev/docs/installation) + iOS SDK

```bash
export THEOS=/opt/theos
cd dylib
make -f Makefile.theos package
# Output: .deb file — install via Cydia / Sileo
```

---

## Dialog appearance

**Dialog 1 — Activation** (like photo 1):
```
┌─────────────────────────────────────────┐
│              @xit1299                   │  ← cyan title
│  Enter your @xit1299 key to continue.   │
│ ┌─────────────────────────────────────┐ │
│ │ Paste license key                   │ │  ← text input
│ └─────────────────────────────────────┘ │
│        [Activate]        [Quit]         │
└─────────────────────────────────────────┘
```

**Dialog 2 — Success** (like photo 2):
```
┌─────────────────────────────────────────┐
│              @xit1299                   │  ← cyan title
│  ✅  @xit1299 VIP activated             │
│  ⏳  23h 59m remaining                  │
│                                         │
│  🔒  Protected by @xit1299              │
│                                         │
│              [Enter App]                │
└─────────────────────────────────────────┘
```

---

## How it works

| Step | What happens |
|------|-------------|
| 1 | App calls `XIT1299_Activate(api_url)` at startup |
| 2 | Dialog 1 — user pastes license key |
| 3 | dylib POSTs `{ key, deviceId }` to `/api/keys/validate` |
| 4 | First use → server binds key to this device permanently |
| 5 | Key valid → Dialog 2 shows success + time remaining |
| 6 | User taps **Enter App** → app continues |
| 7 | Different device → **rejected** |

---

## Device ID sources

| Platform | Source |
|----------|--------|
| macOS | `IOPlatformUUID` (hardware UUID via IOKit) |
| iOS | `UIDevice.identifierForVendor` |

---

## API endpoint used

```
POST /api/keys/validate
Content-Type: application/json

{ "key": "XIT-XXXXXXXX-XXXXXXXX-XXXXXXXX", "deviceId": "MAC:xxxxxxxx-..." }
```
