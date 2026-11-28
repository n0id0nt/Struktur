# How both games behaved before the split (step 6.0)

Recorded 2026-11-27, to compare against after each later step. Indoor Cycling Software was run from `Modules`
(`f70f685`); Seven Gods from `main` (`92457c7`, tagged `pre-split-main`). Both were Debug builds on Windows.

## Builds and tests

`builds\check-all.ps1 -Web` on `Modules`:

| Step | Result |
|---|---|
| Asset path case | no mismatches |
| Job system tests, with and without threads | 41 of 41 each |
| Fitness and ride tests | 110 of 110 |
| Game debug, editor debug, game release, web | all build |

A fresh clone of `Modules`, with every submodule downloaded from GitHub, passes the same steps (web not repeated).

## Indoor Cycling Software

`run_check.ps1 -Scenario ride -GameArguments --demo`

- Free ride, workout and a second free ride each reach the ride screen with the world loaded, cadence present and the
  rider moving.
- The world is unloaded in the menu.
- No crashes; closes cleanly; save folder unchanged.
- One warning: `WrenSlotToVariant: Foreign object extraction requires type context`.

## Seven Gods

### Loading saves into the overworld (`-Scenario overworld`)

| | Player's saves on the day | Editor save "Thomas Testing Battle" |
|---|---|---|
| Room | FinnsWorkshop | FinnsWorkshopUpstairs |
| Visited | Wisperwoodpath1, ShrineUpperHall | HavenVillage, HavenVillageTrainingGrounds, HavenVillageBorderNorth |
| Party | Player | Player, Kira |
| Items | Iron Sword x1, Healing Herb x3 | Healing Herb x3 |
| Gold | 0 | 0 |
| Moves equipped and unlocked | Step, Slash | Step, Slash, Brace |
| Defeated | 0 | 1 |

Both stay in the overworld after 50 seconds.

### Self-playing battles

| Scenario | Result |
|---|---|
| `battle-wolves` | the party wins in 16 s, both wolves defeated |
| `battle-warden` | the party loses in 13 s |

The AI plays both sides, so compare that a fight runs to an end without errors rather than who wins.

### Editor level viewer

The editor opens the project, the level viewer opens `Levels/SevenGods.ldtk` (34 levels) and draws the whole world,
and the editor closes cleanly with no errors.

### Warnings on `main`

- `Wren binding not found: random.Random...` (five). Harmless: the engine is asked first, and Wren's own `random`
  module then supplies them.
- `Script.get: Entity's script is not initialised`, in the overworld.

No crashes in any run.

## After step 6.1

Checked 2026-11-28 on `Modules` (`b844a75`).

| Check | Result |
|---|---|
| `check-all.ps1 -Web` | 10 of 10, including the two new builds with only the ride's libraries |
| Ride check, all libraries | as above, the same one warning, save folder unchanged |
| Ride check, ride libraries only | the same |
| Ride libraries only | the compiler opened no physics, dialogue, flag or Box2D header |
| Script modules built in memory | identical text to the files generated before (21 modules) |
| Editor with simulated devices: Play, Stop, Play, Refresh, Stop, Play | no errors; the binding count stays the same across restarts; closes cleanly |
| Save key | the same bytes as before |

The editor check used a temporary hook that opens the project and drives the toolbar's actions.

## After step 6.2

Checked 2026-11-28 on `Modules` (`f8a5546`). The ride is now `games/IndoorCyclingSoftware`.

| Check | Result |
|---|---|
| `check-all.ps1 -Web` | 10 of 10: jobs, fitness (51) and ride (59) tests, the 19 library combinations, and the ride in debug, editor, release and web |
| Library check against a deliberate use of the UI from core | fails to link, as it should |
| Ride check from the game folder | as above, the same one warning, save folder unchanged |
| Every other ride screen (`ride-screens`) | all open; screenshots as before the move, save folder unchanged |
| Ride art still using placeholders | the same list as before |
| Editor: Play, Stop, Refresh, then a ride | the game and road world draw in the Game Viewport; saves go to the game's `saves\`; no new warnings |
| `builds\build-windows.bat` | the packaged release game starts from `data.pak` and shows device pairing |
| Fresh clone, submodules not fetched | configuring fetches all 11; the ride builds and passes `ride-screens` |
| Configuring after moving a submodule to another commit | the checkout stays where it was put |

The 6.0 totals (110 fitness and ride tests) are now split between the two test programs.

## After step 6.3

Checked 2026-11-28 on `Modules`. Seven Gods is now `games/SevenGods`.

| Check | Result |
|---|---|
| `check-all.ps1 -Web` | 14 of 14: jobs (41 twice), fitness (51) and ride (59) tests, the 24 library combinations, and both games in debug, editor, release and web |
| `overworld`, both saves | the same room, rooms visited, party, items, gold, moves and defeats as the table above; the same warnings; no crashes |
| `battle-wolves`, `battle-warden` | both fights end (won in 11 s, lost in 13 s) with the same warnings and no crashes |
| Game screenshots beside `main`'s | the same picture, apart from animation frames and how far each fight had got |
| Level viewer | opens `Levels/SevenGods.ldtk` (34 levels); its picture matches `main`'s pixel for pixel |
| Editor: Play, Stop, Play from a spot, Refresh, close the viewer | all work and it closes cleanly; the inspector shows each kind of 2D component the scene had |
| Ride checks (`ride`, `ride-screens`) | as before |
| Assets against `main` | identical, apart from the shader sources (now the engine's), `Settings/DialogueEditor.json` and three scripts' `UI` folder case |
| Save files | unchanged by every run |
| Packaged release, from `data.pak` | `overworld` with the current saves loads the same as above |

The editor check used a temporary hook, and saw no particle emitter or custom shader to inspect. It logged `Script
Chicken is being updated before it was initialised` once for each chicken as its room loaded. The code that starts
scripts hasn't changed since `main`; `main`'s editor run didn't load such a room, so there's nothing to compare with.

Not covered: a play session of Seven Gods, and the web builds in a browser.

## After step 6.4

Checked 2026-11-28 in `SevenGods` and `IndoorCyclingSoftware`, each against its `engine/` submodule (`e96a5f9`).

| Check | Result |
|---|---|
| Builds from the new folders | both games in debug, editor, release and web; the ride's tests (59) |
| `overworld` (both saves), `battle-wolves`, `battle-warden` | as the tables above (the wolves fight took 16 s) |
| `ride`, `ride-screens` | as before, saving under "Indoor Cycling Software" |
| The ride's splash screen and main menu | show the new name and fit the window |
| Both editors | open on their own assets, with the copied layout and saves, and close cleanly |
| Save folders | unchanged by every run |

Not covered: cloning a game from GitHub (the repositories didn't exist yet), and the packaged releases from the new
folders.

## After step 6.5

Checked 2026-11-28.

| Check | Result |
|---|---|
| `check-all.ps1 -Web`, with `games/` removed | 14 of 14: jobs, fitness and ride tests, the library combinations, and both game repositories against this engine in debug, editor, release and web |
| `ride-screens`, finding the game beside the engine | as before |
| Fresh clones from GitHub: engine `d4a340b`, both games pinning it | all three configure and build (Debug); the engine's libraries are fetched by configuring |
| `git clone --recurse-submodules` of a game into a deep folder | fails on SDL_mixer's nested test data, with or without long paths enabled; the documented steps don't take it |

Not covered: a play session of Seven Gods, the web builds in a browser, packaged releases from the game folders, the
real trainer, and a web build or editor build from a fresh clone.

## Repeating it

The saves, reports and screenshots are in `baseline\` (not in git). `tools\checks\README.md` explains the checks.

- **`main`** was built from an export in `build-fresh\baseline-main\game` (about 20 GB with the fresh clone), whose
  `third_party` is a link to the fresh clone's. Its fork-based libraries hold the same code `main` gets from
  `third_party/patches`.
- A copy of `main` fails its first launch while it generates its script bindings. Run it once more.
- Editor saves in `saves\` store `flags.sav` as plain text, but the game reads it encoded. The copies in
  `baseline\saves` are converted; the originals are untouched.
- The editor was opened with a temporary hook in the exported copy that picks the project and opens the world.
