# Struktur

A C++ game engine: CMake libraries a game picks from (`cmake/StrukturLibraries.cmake`), Wren scripting, an ImGui
editor. Its games are separate repositories beside this folder (`..\SevenGods`, `..\IndoorCyclingSoftware`), each
holding the engine as an `engine/` submodule. The copies under `games/` go in step 6.5: don't edit them
(`docs/ModuleSplitPlan.md`).

## Working agreements

- Commit and push only when asked; nothing is pushed here without the owner's go-ahead. Commit messages are one simple
  line: no body, no co-author line. Pushed commits that a game points at are never rebased or squashed.
- C++ and Wren: no single-letter names beyond `i`/`j`/`k` and `x`/`y`/`z`; `static` functions rather than anonymous
  namespaces; avoid lambdas, and ask before changing an existing one. More in `docs/CodeStandards.md`.
- Reactions (notifications, UI updates) are driven by engine events, not by polling for changes each frame.
- Format only the lines you change (`.clang-format`).
- A library's use of an optional library goes inside `STRUKTUR_<NAME>`; the engine names no game.
- Mute the game in every test run, never close a game or editor you didn't start, and put save folders back.

## Verifying

`builds\check-all.ps1` builds every library combination and game and runs the tests (`-Web` adds the web builds).
`tools\checks` drives the real games (see its README); `-Game ..\SevenGods` checks a game in its own folder.
