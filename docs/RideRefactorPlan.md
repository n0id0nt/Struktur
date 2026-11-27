# Ride refactor plan

A plan for branch `RideSoftware`. **Steps 1–5 are implemented** (see "What was done" below); step 6, the module and
repo split, is still to be planned. It covers eight requests:
1. Separate the world from the ride service.
2. Put the world and its renderer on the game context, and connect the world to the ECS.
3. Stop compiling tile maps, render layers and LDtk levels.
4. Load the world explicitly from Wren.
5. Make world actors components.
6. Make the camera 3D and drive it from the camera system.
7. Move the trainer data into `Input`.
8. Review the shared pointers, and look at splitting the engine into modules and repos.

Steps 1–6 below are ordered so the game builds and runs after each one. Step 7 (shared pointers) mostly happens
inside the earlier steps. Step 8 (modules and repos) is a separate, larger track.

## What was done

Implemented with the recommended decisions, in the order 1, 2, 4, 3, 5. The game builds and runs after each step.

| Step | Commit |
|---|---|
| 1. Trainer data in `Input` | Read trainer data from input |
| 2. World out of the ride service | Move the world out of the ride service |
| 4. 2D pipeline off | Stop compiling the 2D world pipeline |
| 3. World in the ECS, 3D camera, textures | Make the world entities and use a 3D camera |
| 5. Shared pointers | Replace shared pointers with single owners |

**Where it differs from the plan below:**
- **Order:** step 4 came before step 3, so the new 3D camera and `WorldRenderer` didn't have to coexist with the 2D
  ones.
- **No `STRUKTUR_2D_WORLD` switch.** The 2D files are kept but simply left out of the build lists. A switch that
  could turn them back on would have needed two cameras and `#ifdef`s through shared files, for a configuration
  nobody uses on this branch. Restoring them is a revert of the step 4 commit.
- **`World.load` takes a settings string** (`Ride.worldSettings()`), not a map: foreign methods can't take maps.
- **Biomes and prop types need no retire list:** the world keeps its own copy, fixed from load to unload, and
  releases the streamer (which waits for builds) first.

**Found on the way:** `ChunkStreamer` lost finished chunks whenever builds completed one at a time (it read a task
after erasing it from its list), leaving gaps in the road. That came from the job-system change in phase 3; it's fixed
with a test ("Fix world chunks being lost when builds finish").

**Files no longer compiled** (kept in the repository):
- World and levels: `World/Level`, `World/TileMap`, `World/RenderLayer.h`, `World/Camera2D`,
  `FileLoading/LevelParser`, `Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator`.
- Components: `Sprite`, `SpriteAnimation`, `ParticleEmitter`, `TileMap`, `Camera`, `Shader`, `Level`, `World`,
  `LevelEntityInstance`.
- Systems: `SpriteRenderSystem`, `AnimationSystem`, `ParticleSystem`, `Camera2DSystem`, `ShaderSystem`.
- Rendering: `Renderer/SpriteBatchRenderer` (the old 2D `WorldRenderer`), `TileChunk*`, `FlipBit.h`.
- Bindings: `WrenAnimation`, `WrenRenderer.cpp`, `Wren2DComponents.cpp` (the 2D component bindings, moved out of
  `WrenGameObjectComponents.cpp`).
- Editor: the level viewer (`LevelViewer/*`, `LevelViewerWindow`), the sprite and shader previews and
  `Inspector2DComponents.cpp`.
- Scripts: the Seven Gods scripts are still in `assets/Scripts`, but `Main.wren` no longer imports them.

## Where things stand

- **`RideIntegration::RideService`** (624 lines) does two jobs:
  - **The ride:** settings file, workout library, `RideSession`, events.
  - **The world:** it owns the `RideWorld`, the `DemoPacer`, the biome list, the asset manifest and the
    `RideWorldRenderer`. It builds the world's per-frame input from the session (`UpdateWorld`) and draws it
    (`RenderWorld`).
- **Created as a side effect today:**
  - The world: `StartFreeRide` / `StartWorkout` call `CreateWorld()`.
  - The renderer: on the first `RenderWorld()` (`if (!m_worldRenderer) make_unique...`, then
    `if (!IsInitialised()) Initialise(...)`).
- **`RideWorld::RideWorld`** owns everything for the road:
  - route, elevation, biome sequence and the chunk streamer;
  - the camera (`WorldCamera`), rider animation, actors (`WorldActor` from an `IWorldActorSource`) and banners.
- **`RideWorldInputs`** is built field by field from the session every frame. It holds:
  - speed, chosen grade, ERG, power zone, cadence, heart rate and power/FTP;
  - workout progress, the gap to target and the intensity of the next 60 s.
- **Engine-side 2D world pipeline:**
  - Pieces: `World::Camera` (2D), `CameraSystem` (2D follow, bounds, shake), `Component::TileMap` + `TileChunk*`,
    `World::Level` + `FileLoading::LevelParser` (LDtk), `World::RenderLayer` (on `Sprite`, `ParticleEmitter`,
    `TileMap`), `SpriteRenderSystem` and the 2D `Renderer::WorldRenderer`.
  - Users: the Seven Gods scripts (`GameWorldState`, `CombatState`, `Main` imports) and the editor's
    `LevelViewer` / `InspectorWindow`.

## 1. Trainer data in `Input`

Smallest step, and it removes the world's dependency on `RideSession` before the world moves.

- **Add `Input::Ride` data**, written once per frame and read everywhere else:

  ```cpp
  struct TrainerInput                  // src/Engine/Input/TrainerInput.h
  {
      // Devices (from FitnessManager)
      std::optional<double> powerWatts, cadenceRpm, heartRateBpm;
      bool trainerConnected = false, heartRateConnected = false, powerMeterConnected = false;
      // The ride (from RideSession / WorkoutRunner)
      double speedMetersPerSecond = 0.0;       // simulated
      double chosenGradePercent   = 0.0;       // SIM -/+
      bool ergMode = false;
      int powerZone = 0;
      std::optional<double> powerFraction;     // power / FTP
      std::optional<double> targetDeltaWatts;  // power - target
      double workoutProgress = 0.0;
      std::vector<double> intensityAhead;      // empty = Free Ride
  };
  const TrainerInput& Input::GetTrainer() const;
  void Input::SetTrainer(TrainerInput);        // RideSystem only
  ```

- **Who writes it:** `RideSystem` fills it at the start of the update phase:
  - device fields from `FitnessManager`, even when no ride is running (pairing screens read them too);
  - ride fields from the session.
- **Who reads it:**
  - `World::Update` takes it instead of `RideWorldInputs`, which is deleted.
  - `RideSession::Update` keeps its `SensorReadings` argument for now; its tests drive it directly.
  - The Wren `Ride.power` / `cadence` / `heartRate` getters read `Input` too.
- **Wren:** `Input.trainer` returns a map (or getters on `Input`), so the HUD and the world read one source.
- **A naming caveat:** the device half is real input. The ride half (simulated speed, workout progress) is game
  state derived from input. Keeping both in one struct is what was asked for and keeps a single read path. If the
  split matters later, it's one struct split in two.
- **Tests:** the RideWorld tests build a `TrainerInput` instead of `RideWorldInputs`, a mechanical change.
- **Size:** small.

## 2. The world leaves the ride service

- **New `Struktur::World::World`** (renamed from `RideWorld::RideWorld`, namespace merged into the existing
  `Struktur::World`). It owns the route, profile, biome sequence, chunk streamer, rider animation and banners.
  - It also owns its data: biome definitions, prop types, the asset manifest and the sprite descriptor, loaded by
    `World::Load`. `RideService::LoadWorldData` moves here.
- **`GameContext` gains:**
  - `World::World& GetWorld()` and `Renderer::WorldRenderer& GetWorldRenderer()`. The ride world renderer takes
    over the `WorldRenderer` name; the old 2D one stops compiling in step 4.
  - Both are created with the context, like every other service, in an unloaded state (no chunks, no GPU
    resources). Nothing is created on first use any more.
- **Explicit lifecycle, driven from Wren:**

  ```wren
  // RideState.enter
  World.load({"seed": settings["worldSeed"], "biome": settings["worldBiome"]})  // world + renderer GPU resources
  // RideState.exit
  World.unload()
  ```

  - `World.load` builds the world (the route, the first chunks queued) and calls
    `WorldRenderer::Initialise(manifest, descriptor)`. `World.unload` drops the chunks, waits for in-flight builds
    and releases the GPU resources.
  - `WorldRenderSystem` draws only when the world is loaded, with nothing lazy inside it. Rendering an unloaded
    world asserts in Debug.
  - Settings changes stay explicit: `World.applySettings(map)` (today `RideService::ApplyWorldSettings`).
- **`RideService` keeps** the settings file, the workout library, the session and the events, and never touches
  the world. The link between them is one line in `RideSystem`: after the world updates, it calls
  `session.SetRoadGrade(world.GetRoadGradePercent())`, so SIM still follows the road.
- **Wren `Ride.world*` calls** (`worldActive`, `worldInfo`, `worldJumpTo`, `biomes`) move to a `world` module as
  `World.isLoaded`, `World.info`, `World.jumpTo`, `World.biomes`.
- **Size:** medium. Mostly moving code, plus the Wren lifecycle.

## 3. The world in the ECS

What becomes an entity, and what stays a world resource:

| Thing | Becomes | Why |
|---|---|---|
| The player's rider | entity: `RoadPosition` + `CrankAnimation` + `RiderSprite` | It's a thing in the world with state that systems update. |
| Pacers and ghosts (today `WorldActor`) | entities: `RoadPosition` + `CrankAnimation` + `Tint` | Same; the `IWorldActorSource` interface goes away. |
| The demo pacer | a `PacerSystem` moving pacer entities | Replaces `DemoPacer`; created by `World.load` when `--demo`. |
| Finish banners | entities: `RoadPosition` + `Banner` | Today a `std::vector<double>`. |
| The camera | entity: `Camera3D` + `RoadFollow` | See below. |
| Chunks (terrain, road, props) | stay inside the world (streamer + mesh pool) | Built on workers, recycled hundreds of times a ride. As entities they would churn the registry and need worker-safe structural changes for nothing. The renderer reads them from `World` as now. |
| Textures (manifest roles) | `ResourceManager` textures | `WorldRenderer` loads its role textures with raw bgfx today. They go through `TexturePool` (and `PrefetchTextures`) like every other texture. Placeholders are generated as decoded images and registered by name. |

- **Renames:** `WorldActor` → `RoadPosition` (distance, lateral) + `CrankAnimation` (phase, from cadence) + `Tint`.
  Nothing is called "actor" any more; whether it's a pacer or a ghost is the system that moves it.
- **Systems** (update phase, in order):
  1. `RideSystem` writes `Input` (step 1).
  2. `WorldSystem` advances the world from `Input` and moves the rider entity.
  3. `PacerSystem` moves the pacers.
  4. `RiderAnimationSystem` updates every `CrankAnimation` from cadence (the rider's) or its own pace (pacers'),
     using `SystemAccess::Worker()` from phase 4. It reads `RoadPosition` and writes `CrankAnimation`.
  5. `CameraSystem` (below).

  Render phase: `WorldRenderSystem` queries `RoadPosition` + `CrankAnimation` + `Tint`, and `RoadPosition` +
  `Banner`.
- **Size:** medium.

### The camera

- **`World::Camera` becomes 3D:** position, target, up, field of view, near/far plane, and view and projection
  matrices. The 2D fields (offset, zoom, rotation, the screen/world conversions) go away with the 2D pipeline in
  step 4.
- **`Component::Camera` becomes `Camera3D` + `RoadFollow`:**
  - `RoadFollow` holds the settings: height, distance behind, look-ahead, smoothing, and FOV by speed.
  - The 2D bounds and trauma/shake can return later as 3D shake if wanted.
- **`CameraSystem` is rewritten** around that. `World::UpdateCamera` (smoothed look point, speed-based FOV) moves
  in as the `RoadFollow` behaviour, and the system writes the context's `World::Camera`. `WorldRenderer` reads the
  camera from the context instead of `world.GetCamera()`.
- **The editor's `EditorCamera`** is a 2D pan/zoom camera over levels. It goes with the level viewer (step 4).

## 4. Turn off the 2D pipeline (files kept, not compiled)

- **A CMake switch:** `STRUKTUR_2D_WORLD` (OFF on this branch). It drops these sources from the build:
  - `Component::TileMap`, `TileChunk*` and `FlipBit`;
  - `World::Level`, `FileLoading::LevelParser`, `Component::World` and `Component::LevelEntityInstance`;
  - `World::RenderLayer`, `SpriteRenderSystem` and the old 2D `WorldRenderer`;
  - `TileMapCollisionBodyGenerator`.

  `STRUKTUR_2D_WORLD` is also a compile definition, so the few shared files that mention these types can guard
  them:
  - `Game.cpp` (system registration and LDtk loading);
  - `WrenGameObjectComponents.cpp` (the `Level` / `World` / `TileMap` bindings);
  - `InspectorWindow.cpp`;
  - `DebugSystem.cpp` (tile map collision debug).
- **Components that use `RenderLayer`:** `Sprite` and `ParticleEmitter` have a render layer and are drawn by
  `SpriteRenderSystem`.
  - **Recommended:** turn off the 2D sprite pipeline too: `Sprite`, `SpriteAnimation`, `ParticleEmitter`,
    `SpriteRenderSystem`, `AnimationSystem` and `ParticleSystem`. The ride game draws everything through the world
    renderer and the UI.
  - **Alternative:** keep `Sprite` without its layer and draw sprites as billboards in the world. Not needed today.
- **Editor build:** `LevelViewer`, `LevelViewerWindow`, `EditorCamera` and `EntityFieldEditor` are built on LDtk.
  They drop out under the same switch; the rest of the editor (profiler, file explorer, resource manager) stays.
- **Scripts:** the Seven Gods scripts can't run without levels. On this branch:
  - `Main.wren` stops importing them (`GameWorldState`, `CombatState`, `BattleTestState`, inventory, party, map
    log);
  - the files stay in the repo, untouched, until step 8 moves them to their own game.
- **Box2D and `DebugRenderer`:** they're 2D, but nothing ride-side uses them. They're out of scope for this switch
  and could go under the same flag later.
- **Size:** medium, mostly guarding and building both configurations. The `main` branch keeps
  `STRUKTUR_2D_WORLD=ON`, so Seven Gods is unaffected.

## 5. Shared pointers: review

| Where | What it shares | Needed? | Plan |
|---|---|---|---|
| `Jobs/Task.h` task state | A task's status/result between the running job, every copy of the `Task` handle, its follow-ups and Wren's GC | **Yes.** No single owner: the work, the caller and a script may each outlive the others. | Keep. It's the one deliberate shared-ownership type, and documented as such. |
| `JobSystem::ParallelFor` body | The loop body between the split jobs | **No.** | The final join job owns the body (`unique_ptr` in its work) and frees it after the parts finish; the parts get a raw pointer. |
| `RideSession` save job | The autosave snapshot between the session and its task | **No.** | The lambda owns the job by move (`JobFunction` already supports move-only work). |
| `BiomeList` (`shared_ptr<const vector>`) | Biome definitions between RideService, the world, the streamer and chunk builds on workers | **Not fundamentally.** Builds in flight read the definitions while settings may replace them. | `World` owns the definitions. A replaced set moves to a "retired" list, freed once the builds of its generation finish (the streamer already tracks generations). Builds get a `const*`. Ownership is one place, and the lifetime rule is spelled out. |
| `PropTypeTable` (`shared_ptr<const>`) | Same, for prop types; also copied into the renderer every frame | **Not fundamentally.** | Same as biomes; the renderer gets a `const&` from `World`. |
| `RideService` biome `make_shared` | Loading biomes | **No.** | Goes away with step 2 (`World::Load` owns them). |
| `BleLink` / `DeviceDiscovery` inbox | The message queue between the link and Windows Bluetooth callbacks | **Yes, today.** WinRT can call back after the link object is gone (late disconnect events). | Keep, or make `IBleCentral` promise that `Disconnect()` returns only after its callbacks have finished, then let the link own the inbox. That needs a drain in the WinRT backend, which isn't hardware-verified. Recommend keeping it, with a comment saying why. |
| `WinRtBleCentral` link map | A Bluetooth link between the map and in-flight async GATT operations | **Yes.** Async operations keep using a link removed from the map. | Keep; it's private to the Windows backend. |

So three of the eight go away outright, two become explicit owner-plus-retire-list designs, and three stay with
a stated reason.

## 6. Splitting the engine into modules (and later repos)

The engine now carries two very different games. `GameContext` has getters for fitness and ride services,
`Game.cpp` registers both games' systems, and `RideSoftware` has had to merge from `JobSystem` three times.

### Techniques

- **Modules as CMake libraries with one-way dependencies.** Each area is its own target with a public include
  folder. Engine modules never depend on games; games depend on the modules they use. The pure-C++ pieces
  (`StrukturJobs`, `StrukturFitness`, `StrukturRide`, `StrukturRideWorld`) already work this way, with standalone
  tests.
- **A composition root per game** (the main file you described): each game has a `main.cpp` that builds the
  application from modules. That's how Bevy (`App::new().add_plugins(...)`), Unreal (modules listed per project)
  and many in-house engines do it:

  ```cpp
  // games/Ride/main.cpp
  int main(int argc, char** argv)
  {
      Struktur::Application app(argc, argv);
      app.AddModule<Struktur::CoreModule>();      // window, input, time, jobs, resources, UI, Wren
      app.AddModule<Struktur::World3DModule>();   // World, WorldRenderer, camera, world systems
      app.AddModule<Ride::FitnessModule>();       // FitnessManager, BLE, pairing bindings
      app.AddModule<Ride::RideModule>();          // RideService, RideSystem, ride bindings
      app.SetFrame({Stage::Input, Stage::FixedUpdate, Stage::Update, Stage::Render, Stage::UI});
      return app.Run();
  }
  ```

  A module is an interface with a few hooks, all called by the application at startup:

  ```cpp
  class IModule
  {
  public:
      virtual void RegisterServices(ServiceRegistry& services) = 0;  // what lives on the context
      virtual void RegisterSystems(SystemManager& systems) = 0;      // with SystemAccess, into stages
      virtual void RegisterBindings(Wren::BindingRegistry& bindings) {}
      virtual void RegisterRenderPasses(RenderPasses& passes) {}     // bgfx view order
  };
  ```

- **`GameContext` becomes a service registry.** Today every service is a named getter, so the engine has to know
  about every game. A typed registry (`context.Get<RideService>()`, filled by modules) keeps the engine generic.
  - The core services can keep fast named getters, since they're always there.
- **The frame order belongs to the game:** the stage list and the render pass order (today's fixed `GameLoop` and
  `GraphicsDevice` view ids) come from the composition root. Phase 4's system schedule already works per stage.
- **Wren bindings per module:** `RegisterAllBindings` becomes "each module registers its own". The binding code is
  already split by area (`RegisterRideBindings`, `RegisterFitnessBindings`, ...).
- **Assets per game:** `games/<Game>/assets/`, with the engine's shared assets (fonts, UI kit, shaders) mounted
  first.

### Repos

- **Recommended order:**
  1. **Restructure in this repo first:**
     - `engine/` for the modules;
     - `games/SevenGods/` and `games/Ride/`, each with a main file, a module list and its assets;
     - one CMake project per game, plus the editor.

     The boundaries can be tested without fighting git at the same time.
  2. **Then split:**
     - `Struktur` (engine) becomes its own repo; `git filter-repo --subdirectory-filter engine` keeps its
       history.
     - Each game repo includes the engine as a git submodule, pinned to a commit. That's the pattern
       `third_party/` already uses.
     - Engine changes go to the engine repo; each game bumps its submodule when it wants them.
- **Alternatives:**
  - **CMake FetchContent / CPM pinned to a tag:** simpler for consumers, but awkward when you edit the engine and
    a game together.
  - **git subtree:** no submodule commands, but messier history.

  Submodules fit best, since you develop both sides.
- **Size:** large. Step 1 alone (directories, targets, the `Application` + module interface, the service registry)
  is the biggest single piece of this plan. It's worth doing after steps 1–4, which already cut most of the
  cross-game coupling on this branch.

## Decisions needed

1. **2D sprites on this branch:** turn off the whole 2D sprite pipeline (`Sprite`, `ParticleEmitter`,
   `SpriteRenderSystem`, ...) along with tile maps (recommended), or keep `Sprite` without its render layer?
2. **Seven Gods scripts on this branch:** keep the files but stop importing them (recommended), or delete them
   here?
3. **Shared Bluetooth inbox:** keep it, documented (recommended), or add a callback drain to the WinRT backend so
   the link can own its inbox?
4. **Module split timing:** after steps 1–4 (recommended), or before them?

## Order and checks

| Step | Builds on | Checked by |
|---|---|---|
| 1. Trainer data in `Input` | – | RideWorld/ride tests; demo ride; HUD reads from `Input` |
| 2. World out of the ride service, explicit Wren lifecycle | 1 | Tests; demo ride; a free ride and a workout entered and left several times (no lazy creation, no leaks) |
| 3. Actors, banners, rider and camera in the ECS; textures through `ResourceManager` | 2 | Tests for pacer/rider systems; screenshots match today's |
| 4. `STRUKTUR_2D_WORLD` off | 3 | Game and editor build in both configurations; `main` unaffected |
| 5. Shared pointer changes | inside 2–3 (biomes, prop types), any time (ParallelFor, autosave) | Job and ride tests, stress runs |
| 6. Modules, then repos | 1–4 | A plan of its own when we get there |
