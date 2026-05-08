# XIT1299 License Protection DLL

Protect your Windows application with @xit1299 license keys.
Each key works on **one device only** — bound permanently on first activation.

---

## Quick start

```cpp
#include "xit1299.h"

int main() {
    if (XIT1299_Activate("https://yourapp.replit.app/api") != XIT1299_OK)
        return 1;   // invalid key or user quit
    
    // your app continues here
}
```

Link on Windows: `xit1299.lib`
Link on Linux  : `-lxit1299 -lcurl`

---

## Build — Windows (recommended)

Requires: Visual Studio 2019+ or Build Tools, CMake 3.14+

```bat
cd dll
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output:
- `build\Release\xit1299.dll`  — copy next to your .exe
- `build\Release\xit1299.lib`  — link at compile time
- `build\Release\xit1299_test.exe` — test executable

## Build — Linux (testing only)

Requires: `g++`, `cmake`, `libcurl4-openssl-dev`

```bash
sudo apt install libcurl4-openssl-dev cmake
cd dll
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./xit1299_test https://yourapp.replit.app/api
```

---

## How it works

| Step | What happens |
|------|-------------|
| 1 | App calls `XIT1299_Activate(api_url)` at startup |
| 2 | Dialog 1 appears — user pastes their license key |
| 3 | DLL posts `{ key, deviceId }` to `POST /api/keys/validate` |
| 4 | **First use** → server binds key to this device's hardware ID |
| 5 | Key valid → Dialog 2 shows "@xit1299 VIP activated" + time |
| 6 | User clicks **Enter App** → your app continues |
| 7 | **Different device** → server rejects → user sees error |

---

## Dialog appearance

**Dialog 1 — Activation:**
```
┌────────────────────────────────────────┐  ← dark bg
│ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  ← cyan stripe
│  @xit1299                              │  ← cyan title
│ ────────────────────────────────────── │
│  Enter your @xit1299 key to continue.  │
│                                        │
│  LICENSE KEY                           │
│ ┌──────────────────────────────────┐   │
│ │ Paste license key                │   │  ← edit box
│ └──────────────────────────────────┘   │
│                                        │
│  [     Activate     ] [     Quit     ] │
└────────────────────────────────────────┘
```

**Dialog 2 — Success:**
```
┌────────────────────────────────────────┐
│ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │
│          @xit1299                      │  ← cyan title (centred)
│ ────────────────────────────────────── │
│  ✔  @xit1299 VIP activated             │  ← green
│  ⏳  23h 59m remaining                 │
│                                        │
│  🔒  Protected by @xit1299             │  ← grey
│                                        │
│          [    Enter App    ]           │
└────────────────────────────────────────┘
```

---

## API contract

```
POST /api/keys/validate
Content-Type: application/json

{
  "key":      "XIT-A1B2C3D4-E5F6A7B8-C9D0E1F2",
  "deviceId": "WIN:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
}
```

Response:
```json
{
  "valid":         true,
  "expiresAt":     "2026-05-09T12:00:00Z",
  "timeRemaining": "23h 59m remaining",
  "message":       "@xit1299 VIP activated",
  "deviceLocked":  true
}
```

---

## Device ID sources

| Platform | Source |
|----------|--------|
| Windows  | `HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid` |
| Linux    | `/etc/machine-id` |
| Fallback | Hostname |

---

## Files

| File | Purpose |
|------|---------|
| `xit1299.h`      | Public header — include in your app |
| `xit1299.cpp`    | Full implementation (dialogs + HTTP + device ID) |
| `CMakeLists.txt` | CMake build for Windows and Linux |
| `test_main.cpp`  | Example integration / smoke test |
| `README.md`      | This file |
