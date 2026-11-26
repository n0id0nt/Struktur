# Rider profiles

Startup flow: **Device pairing → Who's Riding? → Main menu → game.**

| Piece | File |
|---|---|
| Profiles model (load/save, current rider, unit conversion) | `assets/Scripts/Rider/RiderProfiles.wren` (`RiderProfiles`, `Units`) |
| "Who's Riding?" list | `States/RiderSelectState.wren` + `UI/Rider/RiderSelectUI.wren` |
| Create / edit a rider | `States/RiderEditState.wren` + `UI/Rider/RiderEditUI.wren` |
| Shared colours, z order, label helper | `UI/Rider/RiderTheme.wren` |

## Data

A profile holds a name (up to 16 characters), a weight, a height and the rider's units (`"metric"` or `"imperial"`).
Profiles are stored in `riders.json` in the save folder, with up to 6 riders. The file also remembers the last
rider chosen, which gets focus next time.

- **Stored in metric.** Weight is kept in kg and height in cm, even for imperial riders. Values are converted only
  for display and editing, so switching units back and forth never changes the stored numbers.
- **Step sizes:** weight moves in 0.5 kg or 1 lb, and height in 1 cm or 1 inch, so every reachable value displays
  cleanly (`72.5 kg`, `175 lb`, `178 cm`, `5' 10"`). Weight is clamped to 30–200 kg and height to 120–220 cm.
- **Serialisation:** the file is written through `MapUtil.mapToPairs`, like every other save. The engine's
  `Json.stringify` stops with a fatal error if it is given a raw map.

Gameplay code reads the chosen rider with `RiderProfiles.current` (a map, or null). It is loaded in `Main.start`
along with the other saves.

## Screens

- **Who's Riding?**
  - **Choosing a rider** makes it current and goes to the main menu.
  - **Edit** sits to the right of each row. **New Rider** is disabled once 6 riders exist.
  - **Back** (UICancel) returns to device pairing.
- **Rider editor:**
  - **Name:** Accept opens the name keyboard. New riders start with it open.
  - **Units, weight and height:** Left/Right changes the value. Weight and height repeat while held, and Accept also
    toggles units.
  - **Buttons:** Save (disabled until there's a name), Cancel, and Delete (confirmed in an `AlertPopup`).
- **Name keyboard** works with either a keyboard or a pad:
  - **Physical keyboard:** typing goes straight in. Enter finishes, Esc cancels, Backspace deletes.
  - **Pad:** the on-screen keys (QWERTY, digits, `'`, `-` and `.`) plus Shift, Space, Del and Done. B cancels.
    Capitals switch on automatically at the start of each word.

## Engine addition: text entry

`Input.startTextInput()`, `Input.stopTextInput()`, `Input.isTextInputActive` and `Input.takeTypedText()` wrap SDL's
text input (`src/Engine/Input/Input.*`, `src/WrenBindings/WrenInput.cpp`).

- **While it's active, keyboard keys stop driving input actions.** Typing a space doesn't also press UIAccept, and
  letters don't trigger bindings. Gamepad and pointer bindings, and raw key queries (`Input.isKeyJustPressed`),
  still work.
- **Stopping text entry is delayed.** The editor waits until Enter and Esc have been released for two frames. That
  stops the keypress that closed the keyboard from also reaching the UI as a UIAccept or UICancel release.
- **Every state that starts text entry must stop it in `exit()`.** Otherwise the keyboard stays cut off from UI
  actions.
