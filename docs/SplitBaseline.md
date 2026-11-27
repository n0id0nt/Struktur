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

## Repeating it

The saves, reports and screenshots are in `baseline\` (not in git). `tools\checks\README.md` explains the checks.

- **`main`** was built from an export in `build-fresh\baseline-main\game` (about 20 GB with the fresh clone), whose
  `third_party` is a link to the fresh clone's. Its fork-based libraries hold the same code `main` gets from
  `third_party/patches`.
- A copy of `main` fails its first launch while it generates its script bindings. Run it once more.
- Editor saves in `saves\` store `flags.sav` as plain text, but the game reads it encoded. The copies in
  `baseline\saves` are converted; the originals are untouched.
- The editor was opened with a temporary hook in the exported copy that picks the project and opens the world.
