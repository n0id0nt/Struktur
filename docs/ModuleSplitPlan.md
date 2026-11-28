# Splitting the engine from the games (refactor step 6)

A plan. Steps 6.0 to 6.2 are done, on the `Modules` branch (record: `docs/SplitBaseline.md`); nothing later is
started. It follows these choices:

- **Three GitHub repositories:** the engine and two games, **Seven Gods** and **Indoor Cycling Software**.
- **Folders:** the engine stays in `Documents\Game Creation Projects\Struktur`; the games go beside it.
- **Engine in a game:** a git submodule.
- **Assets:** anything both games use is copied into each game's repository.
- **The 2D world** is part of the engine, as the `world2d` library.
- **The road world** belongs to the ride game.
- **Libraries:** editor, dialogue, ECS, fitness, flag, physics, UI, Wren and 2D world each get one; everything else
  is core.
- **Libraries that use each other** compile that code out with `#define`s when the other library isn't used.
- **Each game** has its own `Game.cpp` and its own game context.
- **Component list:** broken up, one per library.
- **`main`** takes no changes until step 6.3 is done.

## Still open

| For you | Why |
|---|---|
| Try the ride on the real trainer | Not tested since the refactor, 6.1 or 6.2 |
| Apply or drop the two git stashes (`main`, 27 August: Editor, Game, ImGui renderer; `master`, 14 August: UI batching) | 6.1 and 6.2 moved their files, so they now need applying by hand |
| Confirm level, dialogue and art edits to Seven Gods are also on hold until 6.3 | Code and assets both move in that step |
| Say if the local engine override (below) isn't wanted | 6.2 built it in as recommended |
| `Struktur` is public on GitHub, with the old game assets in its history and the save encoding key in its source | Say if it should be private. The new game repositories will be private |

## Folders and repositories

```
Documents\Game Creation Projects\
  Struktur\                  engine (this folder, the existing GitHub repository)
  SevenGods\                 new repository
    engine\                  git submodule -> Struktur on GitHub
    src\  assets\
  IndoorCyclingSoftware\     new repository
    engine\                  git submodule
    src\  assets\
```

| Goes to | What |
|---|---|
| Engine | the libraries (including the 2D world and its level viewer), `third_party/`, library shaders, tests, `job_bench`, `trainer_probe`, engine docs |
| Indoor Cycling Software | `Ride`, `RideIntegration`, the road `World`, its systems, renderer, camera, shaders and bindings; ride tests and `validate_fit.py`; ride assets and docs |
| Seven Gods | its `Game.cpp` and context, assets, scripts, design docs, `dialogue_to_markup.py` |

- **Assets:** each game gets its own copy of the UI kit art, fonts, input glyphs, input settings and the shared
  scripts (state manager, UI kit, buttons and so on). The two copies can then change independently. The engine
  repository holds no game assets. Shaders compiled into a library stay with that library, since they're its code.
- **History:** the engine repository keeps its history as it is. Nothing is rewritten or force-pushed, so
  `GMTK2025` and the other branches stay intact. Each game repository is made from a fresh clone filtered to that
  game's files with `git filter-repo`, so their history comes along. The one cost: the engine's history still
  contains old game assets (266 MB packed).

## The engine submodule

Each game records the engine commit it uses. There will be three copies of the engine on disk (about 930 MB of
`third_party` each).

- Clone a game with `git clone --recurse-submodules`, since the engine has submodules of its own.
- An engine change made inside a game: switch `engine\` to a branch (a submodule starts on no branch), commit, push
  the engine, then commit the new pointer in the game. Each game sets `push.recurseSubmodules=check`, which stops a
  push that points at an unpushed engine commit.
- Pushed engine commits that a game points at are never rebased or squashed.
- Configuring fetches only the submodules that are missing (6.2), so it never moves a checkout you're working on.
- **Local override:** a CMake setting, `STRUKTUR_DIR`, names the engine to build against. It defaults to this
  repository (`../..`) while the game lives inside it, and to `engine\` from 6.4. Point it at the `Struktur` folder to
  see an engine edit in both games at once, with no push and pull between copies; leave it at the default to check a
  game against its recorded commit.

## Libraries

| Library | Contents | Always needs | Compiled in only when also used |
|---|---|---|---|
| `core` | platform, input, files, resources, rendering, audio, text, events, localization, jobs, debug tools, `GameContext` | – | – |
| `ecs` | systems and scheduling, game objects, transforms, hierarchy, script components | core | physics, wren |
| `wren` | script engine, binding registry, state manager, callbacks, core's bindings | core | ui |
| `ui` | UI elements, UI renderer, UI systems, input glyphs | core, ecs | wren |
| `physics` | Box2D world, physics systems, collision layers | core, ecs | wren |
| `world2d` | sprites, animation, particles, entity shaders, tile maps, LDtk levels, 2D camera and renderer | core, ecs, wren | physics |
| `dialogue` | dialogue manager, registry, substitution | core | wren |
| `flag` | flags and their saving | core | wren |
| `fitness` | Bluetooth, trainer and sensors, `TrainerInput` | core | ecs, wren |
| `editor` | ImGui editor, debug drawing, level viewer | core, ecs | ui, physics, dialogue, wren, world2d |

| Game | Libraries |
|---|---|
| Seven Gods | core, ecs, wren, ui, physics, world2d, dialogue, flag (+ editor) |
| Indoor Cycling Software | core, ecs, wren, ui, fitness (+ editor) |

- `world2d` needs `wren` because a level keeps its entities' fields as script values and builds the entities from
  scripts.
- Jobs was already a separate library; core links it, and its standalone tests and benchmark keep working.
- The startup, frame and shutdown that games share (`Engine/App`) build as one more target, whenever `ecs`, `wren`
  and `ui` are all in.

### The `#define` rules

A game lists its libraries once:

```cmake
set(STRUKTUR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Engine source")
include(${STRUKTUR_DIR}/cmake/Struktur.cmake)
struktur_add_game(IndoorCyclingSoftware
    LIBRARIES ecs wren ui fitness
    ASSETS    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    SOURCES   src/Main.cpp src/Game.cpp ...)
```

- Each listed library defines `STRUKTUR_<NAME>` (`STRUKTUR_PHYSICS`, `STRUKTUR_WORLD2D`, ...) for **every** file in
  that game's build, so a class has the same layout everywhere. Each game compiles the engine in its own build folder.
- Libraries a listed one always needs are added automatically.
- A library's use of an optional one sits inside that define. A library's script bindings live with the library:

  ```cpp
  #ifdef STRUKTUR_WREN
  void RegisterPhysicsBindings(Wren::BindingRegistry& registry);
  #endif
  ```

- Building the engine on its own (the root `CMakeLists.txt`) checks 19 sets: each library with only what it needs,
  the combinations games use and everything together, with and without the editor. Each links every object of the set
  into an empty program, so a missing guard fails there. `builds\check-all.ps1` runs it.

What has to change for the libraries to separate (found by checking includes on `main` and `RideSoftware`):

| Today | Change |
|---|---|
| Core's callback files use Wren, and one uses UI | The Wren files move to the `wren` library; the UI case goes inside `STRUKTUR_UI` |
| `Input` holds `TrainerInput` | Inside `STRUKTUR_FITNESS` |
| Game objects and hierarchy touch physics bodies | Inside `STRUKTUR_PHYSICS` |
| The physics system handles tile maps, and the tile collision builder sits in `Physics` | Both parts move to `world2d`, inside `STRUKTUR_PHYSICS`, so physics doesn't know about tile maps |
| The level viewer and 2D previews are wired into the editor | Inside `STRUKTUR_WORLD2D` |
| The script component registry sits in scripting but needs ECS | Moves to `ecs`, inside `STRUKTUR_WREN` |
| `DebugSystem` is in ECS but needs the editor | Moves to `editor` |
| One `ComponentListXMacro.h` names every library's components | One list per library (below) |
| The dialogue editor fixes the script folder and speaker names (Greg, Mei, Baron, ...) | The game supplies both |
| The key that encodes save files is in the engine's file system code | Each game supplies its own; Seven Gods keeps the current one, so its saves still load |

6.1 made these changes, apart from the tile map and level viewer rows, which concern the 2D code (6.3):

- `Game.cpp` gives the key to `FileSystem::SetSaveKey`.
- The dialogue editor reads `Settings/DialogueEditor.json` from the open project. The file goes with Seven Gods in
  6.3.

### Component lists

Each library keeps its own list beside its components, with its script wrappers. One header joins the lists the game
uses, so the code that reads `COMPONENT_LIST` stays as it is. A game can add its own list without the engine naming
it.

```cpp
// Engine/ECS/ScriptComponents.h (used by the script bindings)
#include "Engine/ECS/EcsComponentList.h"             // ECS_COMPONENT_LIST: transforms, script
#ifdef STRUKTUR_PHYSICS
#include "Engine/Physics/PhysicsComponentList.h"     // PHYSICS_COMPONENT_LIST: PhysicsBody
#else
#define PHYSICS_COMPONENT_LIST
#endif
#ifdef STRUKTUR_GAME_COMPONENT_HEADER
#include STRUKTUR_GAME_COMPONENT_HEADER              // set by the game's CMake; defines GAME_COMPONENT_LIST
#else
#define GAME_COMPONENT_LIST
#endif

#define COMPONENT_LIST ECS_COMPONENT_LIST PHYSICS_COMPONENT_LIST GAME_COMPONENT_LIST
```

6.3 adds the 2D world's list (camera, level, sprite, tile map and so on) the same way.

## Game context

Core declares what the libraries need as virtual getters, and each game implements them and adds its own. Existing
calls such as `context.GetRegistry()` don't change.

```cpp
// Struktur: Engine/GameContext.h
class GameContext
{
public:
	virtual ~GameContext() = default;

	Platform::Window& GetWindow() const;  // core's own services stay in the base class
	Input::Input& GetInput() const;

#ifdef STRUKTUR_ECS
	virtual entt::registry& GetRegistry() const = 0;
#endif
#ifdef STRUKTUR_PHYSICS
	virtual Physics::PhysicsWorld& GetPhysicsWorld() const = 0;
#endif
#ifdef STRUKTUR_WORLD2D
	virtual World2D::Camera2D& GetCamera2D() const = 0;
#endif
};
```

```cpp
// Indoor Cycling Software: src/RideContext.h
class RideContext final : public GameContext
{
public:
	static RideContext& From(GameContext& context) { return static_cast<RideContext&>(context); }

	entt::registry& GetRegistry() const override;
	RideIntegration::RideService& GetRideService() const;  // the game's own additions
	World::World& GetWorld() const;
	World::Camera& GetCamera() const;

private:
	System::Services m_ecs;  // each library supplies a holder for its services
	std::unique_ptr<RideIntegration::RideService> m_rideService;
};
```

- Core only forward-declares library types, so it doesn't depend on those libraries.
- Each game context creates everything in its constructor and destroys it in a stated order. Nothing is created on
  first use.
- Game code reaches its own additions with `RideContext::From(context)`. That cast is safe because a program has one
  kind of context.

## Game.cpp

Each game's `Game.cpp` creates its context, registers its bindings, systems and render views, and writes out the frame.
The engine provides the shared pieces (asset and save folders, window and editor setup, splash screen, input polling,
fixed-step loop, restart and shutdown) as functions in `Engine/App/App.h`, so they aren't copied into each game.

```cpp
// Indoor Cycling Software: src/Game.cpp
static void RegisterBindings(Wren::BindingRegistry& registry)
{
	Wren::RegisterEngineBindings(registry);  // every engine library in the build
	Wren::RegisterRideBindings(registry);
	Wren::RegisterWorldBindings(registry);
}

static void Frame(GameContext& context)
{
	App::BeginFrame(context);
	switch (context.GetGameData().gameState)
	{
		case Core::GameState::SPLASH_SCREEN: App::SplashScreenFrame(context, k_splashScreen); break;
		case Core::GameState::GAME: App::PlayFrame(context); break;
		default: App::StopLoop(context); break;
	}
	App::EndFrame(context);
}
```

Pieces that used to be fixed lists are now the game's to control:

- **Render views:** `GraphicsDevice::AddView` hands them out in draw order. The game adds its own first, then the
  engine adds the editor's debug drawing, the UI and the editor.
- **Shaders:** each library and game lists its own with `struktur_add_shaders` and registers the table. The core and UI
  shaders are in `shaders/`.
- **Script modules:** the `.wren` sources are built in memory from the registry, with no generated files.
- **Editor:** each game builds its own editor program, which opens that game's `assets\`; Play and Stop call functions
  the game gives it. The project launcher is gone.

## Steps

Work happens on a branch, `Modules`, from `RideSoftware` (which already contains `main` and `JobSystem`). `main` is
left as it is until the merge in 6.5. Each step ends with every game that exists at that point building and passing
its checks.

| Step | Work | Done when | Size |
|---|---|---|---|
| 6.0 Groundwork (done) | Tag `main` and `RideSoftware` (`pre-split-...`); fix the `Ui` / `UI` folder case; fork wren and bgfx.cmake and use the forks; record how both games behave now; one script that builds and tests everything | A fresh clone builds; the record exists | Small |
| 6.1 Untangle (done) | Virtual `GameContext` and `RideContext`; per-library component lists; in-memory script modules; the include changes above and their guards | Ride tests and demo ride as today | Medium–large |
| 6.2 Libraries (done) | A CMake target per library and `struktur_add_game`; the ride becomes `games/IndoorCyclingSoftware` (still in this repository) with its own `Game.cpp` | The engine names no game; the library combinations build | Medium–large |
| 6.3 Seven Gods | `world2d` from the 2D files kept uncompiled, plus the code removed from shared files, taken from `main`; `games/SevenGods` with its `Game.cpp`, context, assets and scripts | It reaches the overworld, plays a self-running battle, loads existing saves and opens the level viewer | Large, riskiest |
| 6.4 Move the games out | Push `Modules`; make each game folder a repository with its history and the engine submodule; copy shared assets and the files git doesn't hold (below) | Both build from their new folders | Medium |
| 6.5 GitHub | Push the games to their new repositories; merge `Modules` into the engine's `main` without squashing, with game code removed by ordinary commits | Fresh clones of all three build | Small |

- **Left for 6.3:**
  - The root `assets/` folder now holds only the Seven Gods leftovers on this branch, and doesn't run. 6.3 brings
    `main`'s assets into `games/SevenGods`, removes it, and re-applies two changes made on this branch: the `Ui` →
    `UI` folder case fix and `Settings/DialogueEditor.json`.
  - The uncompiled 2D files in the engine still include the old paths (`Engine/World/Camera.h`, `Engine/Game.h`).
    `world2d` has to become a library in `cmake/StrukturLibraries.cmake`, with its own render views, and compile the
    `shaders/world2d` pair.
  - `check-all.ps1` and `tools\checks` know one game so far.
- **6.3 is where regressions can hide.** With the job system, Seven Gods has only had short overworld runs, and its
  2D code hasn't been compiled since refactor step 4. Compare against the 6.0 record. Once it passes, Seven Gods work
  resumes in `games/SevenGods`, not on the old layout.
- **GitHub:** the forks `n0id0nt/wren` and `n0id0nt/bgfx.cmake` hold the two library fixes on a `struktur` branch. The
  game repositories are created in 6.4. Nothing is pushed to the engine repository without your go-ahead.
- **Switching to `main` or `RideSoftware`** puts wren and bgfx.cmake back to their unfixed versions. Apply the files in
  `third_party/patches` there before building.

### Handled in 6.4

- **Files git doesn't hold:** copy `saves/` (editor snapshots), the editor layout and project files, `imgui.ini` and
  local editor settings to the game that uses them.
- **Save folders:** Seven Gods keeps its name, so its saves carry on. Indoor Cycling Software gets its own folder and
  copies its rides, workouts and input settings from `StrukturGames\Seven Gods` once.
- **Sprite pipeline** (`..\3dModelsToSprites`): point it at `SevenGods\assets`. The ride's copy of the rider sprite
  (`games/IndoorCyclingSoftware/assets/Actors/explorer.png`, since 6.2) is then updated by hand.
- **Working notes:** add a `CLAUDE.md` to each repository, because Claude's memory is tied to this folder.
