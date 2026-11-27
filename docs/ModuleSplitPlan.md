# Splitting the engine from the games (refactor step 6)

A plan. Nothing here is implemented. It follows these choices:

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

## Before starting

| For you | Why now |
|---|---|
| Try the ride on the real trainer, and Play/Stop in the editor | Neither is tested since the refactor. A fault found after the files move is much harder to trace |
| Apply or drop the two git stashes (`main`, 27 August: Editor, Game, ImGui renderer; `master`, 14 August: UI batching) | They won't apply cleanly once the files move |
| Confirm level, dialogue and art edits to Seven Gods are also on hold until 6.3 | Code and assets both move in that step |
| Decide on the local engine override (below) | It shapes how the game projects are set up in 6.2 |
| Keep the game repositories private unless the art and audio licences allow sharing, and check whether Struktur is public | The only licence file in `assets` is for one font, and the engine's history keeps the old game assets |

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
- **To fix first:** the engine's `CMakeLists.txt` runs `git submodule update --init --recursive` on every configure.
  In a game, that would move `engine\` back to the recorded commit while you work on it. It changes to fetch only
  what's missing.
- **Local override (recommended):** a CMake setting, `STRUKTUR_DIR`, that defaults to `engine\` and can point at the
  `Struktur` folder instead. An engine edit then shows up in both games at once, with no push and pull between copies.
  Leave it at the default to check a game against its recorded commit.

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
- Jobs was already a separate library; it folds into core, and its standalone tests and benchmark keep working.

### The `#define` rules

A game lists its libraries once:

```cmake
set(STRUKTUR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine CACHE PATH "Engine source")
include(${STRUKTUR_DIR}/cmake/Struktur.cmake)
struktur_add_game(IndoorCyclingSoftware
    LIBRARIES ecs wren ui fitness
    SOURCES   ${RIDE_SOURCES}
    ASSETS    assets)
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

- A check builds core alone, each game's set and everything together, so a missing guard fails there.

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

### Component lists

Each library keeps its own list beside its components, with its script wrappers. One header joins the lists the game
uses, so the code that reads `COMPONENT_LIST` stays as it is. A game can add its own list without the engine naming
it.

```cpp
// ecs/ScriptComponents.h (inside STRUKTUR_WREN)
#include "ecs/EcsComponentList.h"             // ECS_COMPONENT_LIST: transforms, script
#ifdef STRUKTUR_PHYSICS
#include "physics/PhysicsComponentList.h"     // PHYSICS_COMPONENT_LIST: PhysicsBody
#else
#define PHYSICS_COMPONENT_LIST
#endif
#ifdef STRUKTUR_WORLD2D
#include "world2d/World2DComponentList.h"     // WORLD2D_COMPONENT_LIST: Camera, Level, Sprite, TileMap, ...
#else
#define WORLD2D_COMPONENT_LIST
#endif
#ifdef STRUKTUR_GAME_COMPONENT_HEADER
#include STRUKTUR_GAME_COMPONENT_HEADER       // set by the game's CMake; defines GAME_COMPONENT_LIST
#else
#define GAME_COMPONENT_LIST
#endif

#define COMPONENT_LIST ECS_COMPONENT_LIST PHYSICS_COMPONENT_LIST WORLD2D_COMPONENT_LIST GAME_COMPONENT_LIST
```

## Game context

Core declares what the libraries need as virtual getters, and each game implements them and adds its own. Existing
calls such as `context.GetRegistry()` don't change.

```cpp
// Struktur: core/GameContext.h
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
class RideContext final : public Struktur::GameContext
{
public:
	static RideContext& From(Struktur::GameContext& context) { return static_cast<RideContext&>(context); }

	entt::registry& GetRegistry() const override;
	Ride::RideService& GetRideService() const;  // the game's own additions
	World::World& GetWorld() const;
	World::Camera& GetCamera() const;

private:
	Struktur::Ecs::Services m_ecs;  // each library supplies a holder for its services
	std::unique_ptr<Ride::RideService> m_rideService;
};
```

- Core only forward-declares library types, so it doesn't depend on those libraries.
- Each game context creates everything in its constructor and destroys it in a stated order. Nothing is created on
  first use.
- Game code reaches its own additions with `RideContext::From(context)`. That cast is safe because a program has one
  kind of context.

## Game.cpp

Each game's `Game.cpp` creates its context, registers its libraries' bindings and writes out the frame. Core
provides the shared pieces (window and file setup, splash screen, input polling, fixed-step loop, shutdown) as
functions, so they aren't copied into each game.

```cpp
// Indoor Cycling Software: src/Game.cpp
static void RegisterSystems(System::SystemManager& systems)
{
	systems.AddHelperSystem<System::HierarchySystem>();
	systems.AddHelperSystem<System::TransformSystem>();
	systems.AddFixedUpdateSystem<System::WrenStateFixedUpdateSystem>();
	systems.AddFixedUpdateSystem<System::WrenScriptFixedUpdateSystem>();
	systems.AddUpdateSystem<System::WrenStateSystem>();
	systems.AddUpdateSystem<System::FitnessSystem>();
	systems.AddUpdateSystem<Ride::RideSystem>();
	systems.AddUpdateSystem<World::WorldSystem>();
	systems.AddUpdateSystem<World::PacerSystem>();
	systems.AddUpdateSystem<World::RiderAnimationSystem>();
	systems.AddUpdateSystem<World::CameraSystem>();
	systems.AddUpdateSystem<System::WrenScriptSystem>();
	systems.AddUpdateSystem<System::EventSystem>();
	systems.AddUpdateSystem<System::UISystem>();
	systems.AddUpdateSystem<System::SoundSystem>();
	systems.AddRenderSystem<World::WorldRenderSystem>();  // render order = this order
	systems.AddRenderSystem<System::UIRenderSystem>();
}

static void RegisterBindings(Wren::BindingRegistry& registry)
{
	Wren::RegisterCoreBindings(registry);
	Wren::RegisterEcsBindings(registry);
	Wren::RegisterUIBindings(registry);
	Wren::RegisterFitnessBindings(registry);
	Ride::RegisterBindings(registry);
	World::RegisterBindings(registry);
}
```

Two other pieces move out of fixed lists so a game controls them:

- **Render views and shaders:** a render system asks for the views it needs, in registration order; each library or
  game compiles and registers its own shaders.
- **Script modules:** the `.wren` sources are built in memory from the registry. This removes the generated files,
  the output folder outside the repository and the second run after a binding change.

Each game builds its own editor program, because the editor has to link that game's components. The project
launcher goes.

## Steps

Work happens on a branch, `Modules`, from `RideSoftware` (which already contains `main` and `JobSystem`). `main` is
left as it is until the merge in 6.5. Each step ends with every game that exists at that point building and passing
its checks.

| Step | Work | Done when | Size |
|---|---|---|---|
| 6.0 Groundwork | Tag `main` and `RideSoftware` (`pre-split`); fix the `Ui` / `UI` folder case; fork wren and bgfx.cmake and use the forks; record how both games behave now; one script that builds and tests everything | A fresh clone builds; the record exists | Small |
| 6.1 Untangle | Virtual `GameContext` and `RideContext`; per-library component lists; in-memory script modules; the include changes above and their guards | Ride tests and demo ride as today | Medium–large |
| 6.2 Libraries | A CMake target per library and `struktur_add_game`; the ride becomes `games/IndoorCyclingSoftware` (still in this repository) with its own `Game.cpp` | The engine names no game; the library combinations build | Medium–large |
| 6.3 Seven Gods | `world2d` from the 2D files kept uncompiled, plus the code removed from shared files, taken from `main`; `games/SevenGods` with its `Game.cpp`, context, assets and scripts | It reaches the overworld, plays a self-running battle, loads existing saves and opens the level viewer | Large, riskiest |
| 6.4 Move the games out | Push `Modules`; make each game folder a repository with its history and the engine submodule; copy shared assets and the files git doesn't hold (below) | Both build from their new folders | Medium |
| 6.5 GitHub | Push the games to their new repositories; merge `Modules` into the engine's `main` without squashing, with game code removed by ordinary commits | Fresh clones of all three build | Small |

- **6.3 is where regressions can hide.** With the job system, Seven Gods has only had short overworld runs, and its
  2D code hasn't been compiled since refactor step 4. Compare against the 6.0 record. Once it passes, Seven Gods work
  resumes in `games/SevenGods`, not on the old layout.
- **`git filter-repo`** isn't installed (`pip install git-filter-repo`).
- **GitHub:** the `gh` command isn't installed, so you create the empty repositories (for 6.0, the two forks; for
  6.4, `SevenGods` and `IndoorCyclingSoftware`) or install `gh`. Nothing is pushed without your go-ahead.

### Handled in 6.4

- **Files git doesn't hold:** copy `saves/` (editor snapshots), the editor layout and project files, `imgui.ini` and
  local editor settings to the game that uses them.
- **Save folders:** Seven Gods keeps its name, so its saves carry on. Indoor Cycling Software gets its own folder and
  copies its rides, workouts and input settings from `StrukturGames\Seven Gods` once.
- **Sprite pipeline** (`..\3dModelsToSprites`): point it at `SevenGods\assets`. The ride's copy of the rider sprite is
  then updated by hand.
- **Working notes:** add a `CLAUDE.md` to each repository, because Claude's memory is tied to this folder.
