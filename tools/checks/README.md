# Checks

## Builds and tests

```powershell
powershell -ExecutionPolicy Bypass -File builds\check-all.ps1
```

Builds the standalone test projects, the engine's library combinations and each game in every desktop
configuration, runs the tests and checks the letter case of asset paths. Add `-Web` for the web build. Each step logs to `<build folder>\check.log`.

## Behaviour checks

These start the real game, let it drive itself, and collect a report, screenshots and the distinct warnings it
printed. Run them before and after a change and compare.

| Scenario | Game | What it does |
|---|---|---|
| `ride` | Indoor Cycling Software (add `-GameArguments --demo`) | Free ride, workout and a second free ride on simulated devices |
| `ride-screens` | Indoor Cycling Software (add `-GameArguments --demo`) | Opens every other screen in turn, so a missing script, picture or font shows up |
| `overworld` | Seven Gods | Loads the saves, starts in the overworld and reports room, party, items, moves and defeats |
| `battle-wolves` | Seven Gods | The battle test's first wolves fight, both sides playing themselves |
| `battle-warden` | Seven Gods | The same against the Warden |

```powershell
powershell -ExecutionPolicy Bypass -File tools\checks\run_check.ps1 -Scenario ride -OutDir baseline\ride -GameArguments --demo
```

| Option | Meaning |
|---|---|
| `-Game` | The folder holding `assets\` (default: the scenario's game under `games\`) |
| `-Exe` | The game to run (default: the program in the game's `build-nogame\Debug`) |
| `-Saves` | A folder of save files that replaces the `.sav` files for the run, so two runs start from the same state |
| `-SaveName` | The save folder under `%APPDATA%\StrukturGames` |

A run changes the game's scripts and the save folder, and puts both back when it ends. The save folder as it was is
also kept in `<OutDir>\saves-before-<time>`. Don't play or edit the game while a check runs. A check mutes the game
and captures its window without taking focus.

The scenarios patch exact lines of the scripts (`scenarios.py`), so a script change can need a matching change there.
Results go in `baseline\`, which git ignores.
