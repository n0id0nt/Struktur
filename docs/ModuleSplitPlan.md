# Splitting the engine from the games (refactor step 6)

A plan. Steps 6.0 to 6.4 are done (record: `docs/SplitBaseline.md`); 6.5 isn't started. It follows these choices:

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
- **`main`** took no changes until step 6.3 was done. Work on the games now happens in their own folders, beside
  this one; the copies under `games/` are removed in 6.5.

## Still open

| For you | Why |
|---|---|
| Try the ride on the real trainer | Not tested since the refactor or the split |
| Apply or drop the two git stashes (`main`, 27 August: Editor, Game, ImGui renderer; `master`, 14 August: UI batching) | The split moved their files, so they now need applying by hand |
| Play a stretch of Seven Gods from its own folder | The checks cover loading, battles and the editor, not a play session |
| Approve pushing the engine commits made after the games moved out | `Modules` on GitHub stops at the commit both games use |
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

- Clone a game with `git -c core.longpaths=true clone --recurse-submodules <url>`: the engine has submodules of its
  own, and some nest too deeply for Windows' default path limit.
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

- Building the engine on its own (the root `CMakeLists.txt`) checks 24 sets: each library with only what it needs,
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

6.1 made these changes, and 6.3 the tile map and level viewer rows, which concern the 2D code:

- `Game.cpp` gives the key to `FileSystem::SetSaveKey`.
- The dialogue editor reads `Settings/DialogueEditor.json` from the open project. The file is with Seven Gods.

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

The 2D world's list (camera, level, sprite, tile map and so on) is `Engine/World/World2DComponentList.h`.

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
	virtual World::Camera2D& GetCamera2D() const = 0;
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
| 6.3 Seven Gods (done) | `world2d` from the 2D files kept uncompiled, plus the code removed from shared files, taken from `main`; `games/SevenGods` with its `Game.cpp`, context, assets and scripts | It reaches the overworld, plays a self-running battle, loads existing saves and opens the level viewer | Large, riskiest |
| 6.4 Move the games out (done) | Push `Modules`; make each game folder a repository with its history and the engine submodule; copy shared assets and the files git doesn't hold (below) | Both build from their new folders | Medium |
| 6.5 GitHub | Push the games to their new repositories; merge `Modules` into the engine's `main` without squashing, with game code removed by ordinary commits | Fresh clones of all three build | Small |

- **What 6.3 changed beyond moving files:**
  - The 2D classes took their files' names: `World::Camera2D`, `System::Camera2DSystem` and
    `Renderer::SpriteBatchRenderer`, reached through `GetCamera2D()` and `GetSpriteBatchRenderer()`. Script names are
    unchanged.
  - The 2D world draws in the engine's own game view, so it adds no render views.
  - The editor's play controls gained `reset` (nothing loaded, scripts and physics up), which the level viewer opens
    a world into.
  - Seven Gods' assets, design docs and `dialogue_to_markup.py` are under `games/SevenGods`. The assets are `main`'s,
    less the shader sources (now the engine's), plus the `UI` folder case fix and `Settings/DialogueEditor.json`.
- **Seven Gods work** resumes in `games/SevenGods`, not on the old layout.
- **Known, and older than the split:**
  - A tile map's vertex and index buffers are never freed (`Renderer::DestroyTileChunks` has no caller), so every
    room that loads uses up more of bgfx's fixed number of buffers.
  - The inspector's "Matrix Uniforms" tree of a shader component is never closed (`ImGui::TreePop`).
- **GitHub:** the forks `n0id0nt/wren` and `n0id0nt/bgfx.cmake` hold the two library fixes on a `struktur` branch. The
  game repositories are created in 6.4. Nothing is pushed to the engine repository without your go-ahead.
- **Switching to `main` or `RideSoftware`** puts wren and bgfx.cmake back to their unfixed versions. Apply the files in
  `third_party/patches` there before building.

### What 6.4 did

- **Repositories:** `SevenGods` and `IndoorCyclingSoftware` beside this folder, each on a `main` branch, made with
  `git filter-repo` from `Modules`. Seven Gods keeps the history of `assets/`, its design documents and the dialogue
  tool; the ride's files are followed back through their earlier names. Each tree matched its folder under `games/`
  before the changes below. Neither has a remote yet.
- **Engine:** an `engine/` submodule at the pushed `Modules` commit. `STRUKTUR_DIR` defaults to it.
- **The ride's name:** window, menu title, splash screen and save folder are "Indoor Cycling Software".
- **Files git doesn't hold:** copied, with the originals left where they were: the editor's `saves/` and layout files
  to the game that used them, `.vscode/` to both, and the ride's rides, workouts and input settings into its new save
  folder. The old project launcher's list wasn't copied, nor `.claude/settings.local.json` (permission settings).
- **Sprite pipeline** (`..\3dModelsToSprites`): installs into `SevenGods\assets`. The ride's copy of the rider
  sprite (`assets/Actors/explorer.png`) is updated by hand.
- **Working notes:** a `CLAUDE.md` in each repository.

### Left for 6.5

- Create the two private GitHub repositories and push the games, with `engine/` pointing at a pushed commit.
- Remove `games/` here with ordinary commits. `builds/check-all.ps1`, `builds/format.bat`,
  `tools/check_asset_case.py` and the check defaults still expect the games inside this repository; the asset case
  check in particular doesn't cover the games' own folders.
- Merge `Modules` into `main` without squashing.
