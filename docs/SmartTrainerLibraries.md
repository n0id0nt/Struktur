# BLE library choices (smart trainer support)

Constraint: Struktur is a commercial game, statically linked with the MSVC static runtime (`/MT`), and it targets
Windows desktop plus a web build. Anything adopted must allow closed-source commercial distribution without
relicensing the game. No code was copied from GPL projects such as GoldenCheetah or qdomyos-zwift. The protocol
code is written from the public Bluetooth SIG specifications.

## Specifications used

- Fitness Machine Service (FTMS) v1.0: Indoor Bike Data, Feature, Supported Power/Resistance Range, Control Point,
  Machine Status
- Cycling Power Service / Profile v1.1: Cycling Power Measurement
- Heart Rate Service v1.0: Heart Rate Measurement
- Bluetooth Assigned Numbers (16-bit UUIDs) and the Core Spec's GATT characteristic properties

## Per-platform options

| Platform | Option | Licence | Verdict |
|---|---|---|---|
| Windows | **WinRT `Windows.Devices.Bluetooth` via C++/WinRT** | OS API. C++/WinRT headers ship in the Windows SDK (`Include/<ver>/cppwinrt`); C++/WinRT itself is MIT | **Adopted.** No extra dependency, works in unpackaged Win32 desktop apps (no manifest capability needed), links the umbrella `windowsapp.lib`, compatible with `/MT`. Needs Windows 10 1703+ for `GattSession` and the `GetGattServicesAsync` family. |
| Windows | Win32 `BluetoothGATT*` (SetupAPI) | OS API | Rejected. It only works with already-paired devices and can't scan. |
| Linux | BlueZ over D-Bus | The BlueZ daemon is GPL-2.0, but D-Bus is IPC, so no linking and no copyleft reaches the game. Client libraries: `libdbus` (AFL-2.1 / GPL-2.0 dual, take AFL), `sd-bus` (LGPL-2.1+), GDBus (LGPL-2.1+), `sdbus-c++` (LGPL-2.1) | **Future backend.** Prefer `libdbus` under AFL-2.1, or LGPL libraries linked dynamically. LGPL with static linking would need relinkable objects shipped. |
| macOS | CoreBluetooth | Apple SDK framework | **Future backend.** Needs an Objective-C++ (`.mm`) file and `NSBluetoothAlwaysUsageDescription` in the app's Info.plist. |
| Web | Web Bluetooth (Emscripten JS glue) | Browser API | **Possible future backend.** Chromium-only, and needs a user gesture to pick a device. The web build uses `NullBleCentral` today. |
| Cross-platform | SimpleBLE | Business Source License 1.1 since 20 Jan 2025. Free for non-commercial use; commercial use needs a paid licence; each version converts to GPL-3.0 after 4 years | **Rejected** for a commercial game unless a licence is bought. It also hides the GATT-session details this code needs for clean Direto release. |
| Cross-platform | Qt Bluetooth | LGPL-3.0 / GPL / commercial | Rejected. It pulls in Qt, and LGPL-3 with static linking is awkward. |

## Other dependencies

| Library | Use | Licence | How it is pulled in |
|---|---|---|---|
| doctest 2.4.12 | unit tests only | MIT | `FetchContent`, release tarball pinned by SHA-256 (`73381c7a…2d70`). Only the header is used, and doctest's own CMake project is never added. Not linked into the game. |
| nlohmann/json | device registry | MIT | already a git submodule in `third_party/json` |
| pugixml 1.14 | ZWO workout parsing (ride screens) | MIT | `FetchContent`, release tarball pinned by SHA-256 (`2f10e276…a015`). `pugixml.cpp` is compiled into `StrukturRide`. |

The ride screens' font (Barlow Semi Condensed, OFL), the test-only `fitdecode` and the decision to write our own FIT
encoder are recorded in [RideScreens.md](RideScreens.md#library-choices).

## Design consequences

- `IBleCentral` is the seam. Linux, macOS and web backends would each be one more implementation, selected in
  `Ble/BleCentralFactory.cpp`.
- C++/WinRT headers are compiled in one translation unit only (`WinRtBleCentral.cpp`, behind a pimpl). The rest of
  the engine never sees them, and `/bigobj` is set on that one file.
- ANT+ FE-C is out of scope. Note that the ANT+ SDK and the FE-C device profile have their own licence (an ANT+
  Adopter agreement), which needs checking before that work starts.
