# Smart trainers, heart rate and power meters (BLE)

`src/Engine/Fitness/` (library `StrukturFitness`) connects the game to:

| Role | BLE service | Tested against |
|---|---|---|
| Smart trainer | Fitness Machine Service (FTMS) `0x1826` | Elite Direto, Wahoo KICKR v4 (2018, firmware ≥ 3.5.1) - **mock only so far** |
| Heart rate monitor | Heart Rate `0x180D` | any standard strap - **mock only so far** |
| Power meter | Cycling Power `0x1818` | any standard meter - **mock only so far** |

The backend is Windows-only for now (WinRT). Every other platform builds with a null backend that reports
Bluetooth as unavailable. See [SmartTrainerLibraries.md](SmartTrainerLibraries.md) for why.

## Architecture

```
 game thread                                                         BLE threads
 ───────────                                                         ───────────
 FitnessSystem (ECS update system)
   └─ FitnessManager ── owns ──► IBleCentral ◄── WinRtBleCentral (worker thread + WinRT thread pool)
        ├─ DeviceScanner               ▲            FakeBleCentral  (tests, trainer_probe --simulate)
        ├─ DeviceRegistry (JSON)       │            NullBleCentral  (web / Linux / macOS)
        ├─ FtmsTrainer ─┐              │ callbacks only push into ThreadSafeQueue inboxes
        ├─ BleHeartRateSensor ─┼─ BleLink ◄──┘ (drained on the game thread in Update)
        └─ BlePowerMeter ──────┘
                    │
                    ▼ pure, I/O-free
        Protocol/  Ftms · HeartRate · CyclingPower · CadenceCalculator · Bytes · BleUuid
```

**Layers, bottom up**

1. **`Protocol/`** has pure parsers and command builders with no BLE and no clock. They are unit-tested with golden
   byte arrays built from the Bluetooth SIG field tables.
   - `Ftms`: Indoor Bike Data `0x2AD2` (bit 0 "More Data" is inverted, so speed is present when it is *clear*),
     Feature `0x2ACC`, Supported Power Range `0x2AD8`, Supported Resistance Range `0x2AD6`, the Control Point
     `0x2AD9` builders (Request Control, Reset, Start/Resume, Stop/Pause, Set Target Power, Set Target Resistance
     Level, Set Indoor Bike Simulation Parameters), Response Code parsing, and Machine Status `0x2ADA`.
   - `HeartRate`: `0x2A37` 8- and 16-bit values, sensor-contact bits, energy, and RR intervals.
   - `CyclingPower`: `0x2A63` with every optional field in the spec's fixed order.
   - `CadenceCalculator`: cadence from crank revolution and event-time deltas, with uint16 rollover handling,
     rejection of implausible values, decay to 0 when pedalling stops, and a re-baseline after long gaps.
2. **`Ble/IBleCentral`** is the minimal central-role interface: scan by service UUID, connect, discover, read,
   subscribe, write, and disconnect. Callbacks may arrive on any thread, so the contract is to push and return.
   Implementations:
   - `WinRtBleCentral` runs all GATT operations one at a time on a worker thread over C++/WinRT. On disconnect
     it writes CCCD = none, revokes its handlers, releases the `GattSession`, closes every service and closes
     the device.
   - `FakeBleCentral` defers every callback until `Pump()`, which makes tests deterministic.
     `FakeFtmsTrainer` is the scriptable mock trainer. It enforces the Request Control gate, gates op codes
     on its features, checks parameter lengths, and can be told to ignore commands, override result codes or
     revoke control.
   - `NullBleCentral` covers platforms with no backend.
3. **`Devices/`** is game-thread state machines with time passed in as `now`, so every timeout is testable.
   - `BleLink` handles one peripheral: connect → discover → setup steps (reads and subscriptions, each required
     or optional) → ready. It reconnects with exponential backoff (1 s, 2 s, 4 s … capped at 30 s) and tags every
     callback with a generation, so late results from an abandoned attempt are ignored.
   - `ControlPointQueue` allows one FTMS procedure in flight at a time. It writes, waits for the matching Response
     Code indication, times out after 5 s by default, and coalesces queued targets (a newer Set Target Power
     replaces one that hasn't been sent yet).
   - `FtmsTrainer`: see "Trainer behaviour" below.
   - `BleHeartRateSensor` and `BlePowerMeter` each subscribe to one characteristic and keep a latest-sample
     snapshot.
   - `DeviceDiscovery` assigns roles by advertised service. A trainer that also advertises Cycling Power (both
     target trainers do) gets roles `[trainer, powerMeter]`, with trainer as the primary role.
   - `DeviceRegistry` remembers the chosen device for each role, by address, as JSON.
4. **Transport-agnostic interfaces.** `Trainer`, `HeartRateSensor`, `PowerMeter`, `TrainerSample`,
   `SimulationTarget`, `CommandResult` and `DeviceEvent` in `DeviceTypes.h` never mention BLE or FTMS. An
   ANT+ FE-C trainer would be another `Trainer` implementation next to `FtmsTrainer`, and
   `RememberedDevice::transport` already distinguishes `"ble"` from a future `"ant"`.
5. **`FitnessManager`** owns the central, the scanner, the registry and one device per role. It refuses to use one
   peripheral for two roles, because that would mean two links to one device.
6. **Engine hook-up:**
   - `GameContext` owns an idle `FitnessManager`.
   - `Game.cpp` sets the registry path to `<save dir>/fitness_devices.json` and loads it. Loading only reads the
     file; nothing connects.
   - `FitnessSystem` pumps the manager every frame, ahead of `WrenScriptSystem` and `EventSystem`, and forwards
     events (see below).
   - `GameContext::Shutdown` tears the manager down first, so the trainer gets its Reset while everything else is
     still alive.

### Trainer behaviour (`FtmsTrainer`)

- **Setup on every (re)connect:**
  1. Read Feature (required), Supported Power Range and Supported Resistance Range (optional).
  2. Subscribe to Indoor Bike Data (notify, required), Control Point (indicate, required) and Machine Status
     (notify, optional).
  3. Send Request Control, then Start/Resume, then re-apply the last target the game set.
- **Capability-driven:** commands are gated on the Feature bits. When the trainer answers `Op Code Not Supported`,
  that capability is switched off. The game gets one `COMMAND_UNSUPPORTED` event and every later call returns
  `UNSUPPORTED`. Resistance control is never assumed: `SetManualResistance(0..1)` maps onto simulation grade (0–10 %
  by default) when the trainer supports simulation, and onto resistance level only if simulation is missing.
- **Clamping:** power is clamped to the trainer's Supported Power Range, snapped to its increment, and then capped by
  `FtmsTrainerSettings::maximumTargetPowerWatts` (the safety cap wins over the trainer's minimum). Resistance is
  clamped to the advertised range and the wire field's 0–25.5. The result reports `CLAMPED`, and the trainer emits
  `TARGET_CLAMPED`.
- **Targets before connecting** return `DEFERRED` and are applied once control is granted.
- **Failures surface as events and are never assumed fixed:**
  - A refused Request Control raises `CONTROL_DENIED`.
  - Machine Status "control permission lost", or a `Control Not Permitted` reply, raises `CONTROL_LOST` and triggers
    an automatic re-request (at most twice before giving up).
  - A command with no reply raises `COMMAND_FAILED`, and three in a row raise `TRAINER_UNRESPONSIVE`. This is how
    the reported "Direto X ERG does nothing" case would show up.
- **Stop / shutdown:** if control is held, the trainer sends `Reset`, which releases control and drops any ERG load.
  It waits up to 2 s for the response and then drops the link. Destroying a device always drops its link.

### Threading

BLE callbacks never touch device state. They push a message into a bounded `ThreadSafeQueue` (when it's full, the
oldest message is dropped), and `Update(now)` drains that queue on the game thread. The central must outlive every
link. `FitnessManager` declares the central first, so it is destroyed last.

## Using it from the game

### Device pairing screen (first screen)

The game opens on `States/DevicePairingState.wren` (view: `UI/Pairing/DevicePairingUI.wren`), before the main menu,
the way cycling apps open on a pairing screen:

- **Tiles:** there are three, Smart Trainer, Power Meter and Heart Rate. Each shows its connection state, the device
  name, and a live value (watts or bpm). Choosing a tile opens a picker listing what the scan found for that role.
  A device already serving another role is listed but disabled; one Bluetooth link per device. The picker also offers
  **Unpair** when the role has a device.
- **Let's Ride** stays disabled until a **power source**, a trainer or a power meter, is connected. Heart rate is
  optional. It then leads to `MainMenu` (`NEXT_STATE` in the state).
- **On entering** the screen, remembered devices reconnect on their own and a scan starts. The scan stops when the
  screen is left; paired devices stay connected.
- **Event-driven:** connection events refresh a tile, `fitnessSample` updates its value, and `fitnessDeviceFound`
  refreshes an open picker. Clicks are queued and handled in `update()`, because they arrive inside the UI input pass.
- **Without hardware:** set `STRUKTUR_FITNESS_SIMULATE=1` before launching the game. `GameContext` then uses
  `SimulatedBleCentral` (a fake trainer, strap and power meter that advertise and stream data on their own). The
  screen's status line says so.

```powershell
$env:STRUKTUR_FITNESS_SIMULATE = '1'; ./build-nogame/Debug/Struktur.exe
```

### Wren API (`import "fitness" for Fitness`)

Roles are the strings `"trainer"`, `"heartRate"` and `"powerMeter"`. Every call returns a fresh snapshot, and nothing
calls back into Wren. Call these in response to the events below rather than every frame.

| Call | Returns |
|---|---|
| `Fitness.isAvailable()` / `Fitness.isSimulated()` | Bool |
| `Fitness.startScan()` / `stopScan()` / `isScanning()` | - / - / Bool |
| `Fitness.devices()` | `[{"address", "name", "rssi", "roles": [role...], "inUseAs": role or null}]` |
| `Fitness.assign(role, address, name)` | Bool. Connects now and remembers the choice in `fitness_devices.json` |
| `Fitness.unassign(role)` | Disconnects (a trainer gets Reset first, which can block for up to ~2.5 s) and forgets the device |
| `Fitness.connectRemembered()` | Starts every remembered device that isn't already running |
| `Fitness.remembered(role)` | `{"address", "name"}` or null |
| `Fitness.status(role)` | `{"state", "name", "address"}`, plus `"control"` and `"mode"` for the trainer, or null |
| `Fitness.sample(role)` | the latest values, using the same keys as `fitnessSample` |
| `Fitness.hasPowerSource()` | Bool: a trainer or power meter is connected |
| `Fitness.setTargetPower(w)` / `setGrade(pct)` / `setManualResistance(f)` / `clearTarget()` | `"queued"`, `"clamped"`, `"deferred"`, `"unsupported"`, `"controlDenied"` or `"noTrainer"` |

From C++ the same comes from `context.GetFitnessManager()`.

### Events

Events reach Wren `onEvent` through the existing event system, so nothing needs to poll:

| Event type | When |
|---|---|
| `fitnessConnected` / `fitnessDisconnected` / `fitnessReconnecting` / `fitnessConnectionFailed` | link lifecycle |
| `fitnessSample` | new data. Carries `power`, `cadence`, `speed` and `heartRate` (trainer), `bpm` (heart rate), or `power` and `cadence` (power meter) |
| `fitnessControlGranted` / `fitnessControlDenied` / `fitnessControlLost` / `fitnessTrainerUnresponsive` | trainer control |
| `fitnessCommandSucceeded` / `fitnessCommandFailed` / `fitnessCommandUnsupported` / `fitnessTargetClamped` | target commands |
| `fitnessDeviceFound` | while scanning, once per newly seen (or newly named) device: `address`, `name`, `rssi`, `roles` |

Every device event's data map has `role` (`trainer`, `heartRate` or `powerMeter`), `detail`, `device` (the address)
and `name`.

## Building and testing

The library configures on its own, which keeps the test loop fast. SDL, bgfx and Wren are not needed.

```powershell
cmake -S src/Engine/Fitness -B build-fitness -G "Visual Studio 17 2022" -A x64
cmake --build build-fitness --config Debug
./build-fitness/Debug/StrukturFitnessTests.exe
```

- doctest is fetched by FetchContent, pinned to v2.4.12 with a SHA-256.
- The tests cover golden-byte parsing for every characteristic above, and the control point queue rules.
- They exercise the trainer end to end against `FakeFtmsTrainer`: connect, control, clamping, unsupported
  ops, manual resistance, denial, revocation, unanswered ERG, reconnect, connect backoff, a single-connection
  trainer held by another app, Reset-on-stop, and More Data merging.
- They also cover the HR and power sensors, scanning, the registry, and `FitnessManager` shutdown.
- In the game build (root `CMakeLists.txt`) the library is linked, and the tests and probe are switched off with
  `STRUKTUR_BUILD_TESTS` / `STRUKTUR_BUILD_TOOLS` = `OFF`.

## Running `trainer_probe`

Build it as above. The probe is on by default in the standalone build, and its source is `tools/trainer_probe/main.cpp`.

```powershell
# 1. Scan (8 s) and list devices with their roles
./build-fitness/Debug/trainer_probe.exe

# 2. Connect a trainer and watch live data (Ctrl+C to stop cleanly)
./build-fitness/Debug/trainer_probe.exe --trainer AA:BB:CC:DD:EE:FF

# 3. ERG at 80 W (default cap is 100 W)
./build-fitness/Debug/trainer_probe.exe --trainer AA:BB:CC:DD:EE:FF --target-power 80

# 4. Above 100 W needs an explicit flag (optionally with your own cap)
./build-fitness/Debug/trainer_probe.exe --trainer AA:BB:CC:DD:EE:FF --allow-high-power --max-power 250 --target-power 200

# 5. Simulation grade / manual resistance, with a heart rate strap alongside
./build-fitness/Debug/trainer_probe.exe --trainer AA:BB:CC:DD:EE:FF --grade 4 --hr 11:22:33:44:55:66

# 6. Scan, then connect the first device of each kind; or reuse remembered devices
./build-fitness/Debug/trainer_probe.exe --auto --target-power 60
./build-fitness/Debug/trainer_probe.exe --remembered fitness_devices.json

# 7. No hardware: fake trainer + strap + power meter
./build-fitness/Debug/trainer_probe.exe --simulate --target-power 90 --duration 10
```

**Safety cap.** Without `--allow-high-power`, the probe refuses any `--target-power` or `--max-power` above 100 W at
argument parsing, and also passes the cap into `FtmsTrainerSettings::maximumTargetPowerWatts`. That means the trainer
layer clamps any target to the cap regardless of how it was set.

**Output.** Each event prints a line (`trainer controlGranted`, `trainer commandSucceeded Set Target Power`, and so
on), and a status line prints once per second with connection, control state, mode, power, cadence, speed, heart
rate and power-meter values. The engine's `INFO:` and `WARNING:` log lines are interleaved.

**Exit codes:** `0` OK, `1` bad arguments or address, `2` no usable Bluetooth LE adapter.

**Stopping.** Ctrl+C, or the end of `--duration`, sends FTMS `Reset` to the trainer and closes every link. Close the
probe before opening Zwift or the Elite app: the Direto accepts only one BLE connection.

## Hardware test checklist (not verified - no hardware was available)

Everything below has only been run against the mock. **What ran for real:** the WinRT backend found the adapter on
the dev PC, started and stopped a scan (no fitness devices were in range) and shut down cleanly. The editor also
started and exited cleanly with the manager constructed.

1. **WinRT GATT path, all of it:** connect, uncached service and characteristic discovery, CCCD writes for notify
   and indicate, value-changed delivery, write-with-response, and `ConnectionStatusChanged` detecting a powered-off
   trainer.
2. **Link release:** after Ctrl+C or game exit, how quickly another app (Elite app, Zwift) can connect to the
   **Direto**. Windows decides when the radio link actually drops once our handles close. Also check whether a
   Windows-paired (bonded) trainer behaves differently from an unpaired one.
3. **Direto:**
   - Feature bits as reported (`trainer connected (ERG …, simulation …, resistance …)` in the log).
   - Whether Supported Power Range and Machine Status exist.
   - That **Request Control, Start/Resume and Set Target Power each get a Success response** (watch for
     `commandSucceeded Set Target Power`), and that resistance actually changes in ERG.
   - Simulation grade response and feel.
4. **KICKR v4:**
   - With firmware ≥ 3.5.1, the same checks as the Direto.
   - With older firmware (no FTMS), expect `required characteristic 0x2ACC not found` followed by `reconnecting`
     at most every 30 s. Retries are unlimited by default. Confirm that message.
5. **Reset on stop** drops the ERG load on both trainers, and leaves them usable by the next app.
6. **Control lost:** with a second app taking control (the KICKR allows 3 connections), check for
   `controlLost`, then re-request, then `controlDenied` after repeated loss.
7. **Indoor Bike Data** from both trainers: which fields are flagged, whether records are split with More Data, and
   whether cadence and speed look right.
8. **Advertising:** both trainers must advertise `0x1826` in their advertisement or scan response, otherwise the
   service-filtered scan won't list them. Also check that names appear (active scanning is used).
9. **Heart rate straps:** 8- and 16-bit formats, the contact bit, and RR intervals.
10. **Power meters:** crank-derived cadence against the head unit, and decay to 0 when coasting.
11. **Timing:** whether the 5 s control-point timeout and the 1→30 s reconnect backoff feel right on real hardware.
12. **Pairing screen with real devices:** it has only run against `STRUKTUR_FITNESS_SIMULATE`. Check how quickly
    real devices appear in the picker, how the tiles behave while a trainer reconnects, and the freeze while Unpair
    resets a trainer.

## Decisions and open questions

**Decisions:**
- FTMS only. The Wahoo proprietary protocol is not researched. Propose it only if FTMS control on the KICKR v4
  proves inadequate.
- Windows backend first, behind `IBleCentral`. Linux (BlueZ over D-Bus) and macOS (CoreBluetooth) are separate
  backends to add later.
- Simulation grade is preferred for "manual resistance". Resistance-level control is used only when simulation is
  missing.
- One outstanding control-point procedure, a 5 s timeout, and coalesced targets.
- Reset on stop, so the trainer is not left in ERG at the game's last wattage.
- A target set while disconnected is remembered and re-applied after a reconnect.

**Open questions:**
1. **Set Target Resistance Level encoding.** This implements FTMS v1.0's `uint8`, 0.1 resolution (max 25.5). Some
   trainers may expect `sint16`. It only matters for trainers that advertise resistance-target support. Check the
   feature bits on both trainers.
2. **A game-wide power cap.** The game currently uses only the trainer's own range (`maximumTargetPowerWatts` is
   unset). Should there be a player setting?
3. **Pairing gate:** the game currently can't get past the pairing screen without a power source (or
   `STRUKTUR_FITNESS_SIMULATE`). Do you want a keyboard or "ride without a trainer" fallback?
4. `GameContext` constructs the WinRT backend at startup (a worker thread plus an adapter query, with no radio
   activity). Would you prefer it created lazily, on first use?
5. In editor builds the registry follows the selected save folder (`<saves>/Current`). Is that the place you want it?
6. The game link shows `LNK4098: defaultlib 'LIBCMT' conflicts`. `trainer_probe` links the same library and
   `windowsapp` without it, so it comes from another game dependency, but this hasn't been confirmed against a build
   of `main`.
