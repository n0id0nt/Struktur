# Level Viewer – Design & Phased Plan

Goal: open an LDtk world (`assets/Levels/*.ldtk`) in the editor **without pressing Play**, see every room's
tiles plus each entity's real in-game sprite (no gameplay logic), and fly around it with an editor camera.

---

## What the engine already gives us

| Piece | Where | Relevance |
|---|---|---|
| LDtk parse → `LevelParser::World` | `src/Engine/FileLoading/LevelParser.*` | Already parses all levels, layers, tiles, entity instances + fields. Reuse as-is. |
| Level → ECS entities | `World::Level::CreateWorldEntity` / `LoadLevelEntities` (`src/Engine/World/Level.cpp`) | Builds tilemaps, Collision physics body, and a `WrenScript` per LDtk entity. Needs a "preview" mode. |
| Editor "stopped" state | `debugSettings.playingGame` gates `systemManager.Update/FixedUpdate` in `GameLoop` (`src/Engine/Game.cpp`) | Render systems (`SpriteRenderSystem`, `DebugSystem`) still run when stopped → anything we put in the registry gets drawn, nothing simulates. This is the viewer's home. |
| Camera | `context.GetCamera()` (`World::Camera`: target/offset/zoom) | Written only by `CameraSystem` (an Update system → idle while stopped). An editor camera can write it directly. |
| Viewport | `GameViewportWindow` (framebuffer, hover/focus, `GetGameMousePosition`) | Viewer renders into the same viewport. |
| Stop = full teardown | `StopDebugGame` → `ClearGameSystems` | Clears registry, physics, **shuts down the Wren VM** and asserts the game resource pools are empty. The viewer must boot its own minimal Wren VM and must leave the pools clean. |

Worlds today: `MemoryPalace.ldtk` (9 levels, entities NPC/Item/Puzzle) and `SevenGods.ldtk` (10 levels,
Chicken/Chinlin), both `Free` layout. Note `Puzzle` has no registered script → needs a fallback visual.

---

## Key design decisions

### 1. Viewer = a third editor mode, sharing the Game Viewport
`Stopped → LevelView → Playing`. Entering LevelView populates the normal registry; pressing Play runs the
existing `StartDebugGame`, whose `ClearGameSystems` already wipes the viewer cleanly. Stop returns to
LevelView (re-opening the last file) if it was active before Play.

*Rejected:* a separate registry/window. Every renderer and system reads `context.GetRegistry()` and
`context.GetCamera()`; a second world would mean threading a registry/camera through the whole render path.

### 2. Entity sprites via a **static** `preview(entity, args)` hook, not the constructor
The constructors aren't side-effect free — e.g. `Critter.new` appends to `Critter.all` and consults
`Defeated`; `start()` creates physics bodies, reads flags, wires `Signals`, starts FSMs. So the viewer
never constructs the game object. Instead each GameObject script may declare:

```wren
// Visual only: called by the editor's level viewer instead of new()/start(). Create Sprite (+ optional
// default animation) on `entity` from the LDtk `args`; touch nothing else.
static preview(entity, args) { ... }
```

C++ calls it once per entity, then the entity keeps only Transform + Sprite (+ SpriteAnimation) — no
`WrenScript` component, so no update path can ever reach it. Scripts without the hook (or unregistered
identifiers like `Puzzle`) get a fallback: LDtk-coloured rectangle of the entity's LDtk width/height + name label.

### 3. Editor camera writes `World::Camera` directly
Pan (middle/right drag), zoom-to-cursor (wheel), frame-all (F), frame-level (double-click in list).
Input read from ImGui IO only while the viewport is hovered — the game `Input` system is cleared on
Stop and shouldn't be involved.

### 4. Load all rooms; stream only if worlds grow
At 9–10 rooms, loading everything is cheap and simplest. `Component::World::loadedLevels` already tracks
load state, so view-rect streaming can be bolted on later (Phase 5) without redesign.

---

## Phases

Each phase ends in something visible and testable in the editor.

### Phase 0 — Groundwork (small)
- Add `EditorMode { Stopped, LevelView, Playing }` (or a `levelViewActive` flag) to `EditorSettings`;
  make the toolbar aware of it.
- Split `LoadLevelEntities` behaviour behind a `LevelLoadOptions` struct:
  `createPhysics` (false for viewer), `entityMode { Script, Preview, None }`. Game callers keep current
  behaviour by default. Pull the field→`WrenItem` conversion out into its own function (Phase 3 reuses it).
- Parser: also read each entity def's `color` (and keep `width/height/pivot`, already parsed) for the fallback rect.
- **Done when:** game still runs identically (regression check on both worlds).

### Phase 1 — Static tiles in the viewport
- New `src/Debug/Editor/LevelViewer/LevelViewer.{h,cpp}` (owns: current file path, world entity, mode
  enter/exit) and `Windows/LevelViewerWindow.{h,cpp}` (lists `assets/Levels/*.ldtk`, "Open" button).
- Open: `ClearGameSystems` (if needed) → `CreateWorldEntity` → `LoadLevelEntities(i, {createPhysics=false,
  entityMode=None})` for every level.
- Also open from `FileExplorerWindow` on double-click of a `.ldtk`.
- Exit/Play: teardown through the existing `ClearGameSystems` path; confirm the resource-pool asserts pass.
- **Done when:** both worlds' tiles render in the Game Viewport while stopped (camera fixed for now).

### Phase 2 — Editor camera
- `LevelViewer/EditorCamera.{h,cpp}`: target/zoom state, pan, zoom-around-cursor using
  `World::Camera::ScreenPosToWorldPos`, clamp zoom, frame-rect helper.
- Driven from `LevelViewerWindow`/`GameViewportWindow` hover state; writes `context.GetCamera()` each
  editor frame (target, offset = viewport centre, zoom).
- Auto-frame the whole world on open; remember camera per file for the session.
- **Done when:** you can fly around and zoom both worlds smoothly; Play/Stop doesn't leave the game camera broken.

### Phase 3 — Entity sprites (the core ask)
- **Minimal Wren boot for the viewer:** init `WrenScriptEngine` and register component scripts *without*
  starting `Game`/states. *Phase 1 finding:* `LevelViewer::Open` already calls `StartGameSystems`, which runs
  `WrenStateManager::Initialise` (constructs `Game` from Main.wren - registering every GameObject script) and
  `LoadAllScriptComponents`, but never `Start()`. So the scripts are likely already loaded while viewing -
  verify that first; only split the registration list out of `Main.wren` if it turns out they aren't.
- `WrenScriptComponent`: cache an optional static `preview(_,_)` call handle on the class (alongside the
  existing start/update handles).
- `entityMode = Preview`: per LDtk entity → create GameObject + Transform → build args map (shared
  conversion from Phase 0) → call `Class.preview(entity, args)` → if missing/error, emit fallback rect +
  label (DebugRenderer, or a 1×1 white texture sprite tinted with the LDtk colour).
- Add `static preview` to: `Chicken`, `Chinlin` (via `Critter` helper), `NPC` (uses `NPCData` → `*Sprite.create`),
  `Item` (move the name→sprite-index map to a static), `Sign`, `Door` (closed frame), `PressurePlate`.
  Keep each one to "load texture, `Sprite.create`, set default animation".
- Script errors in a preview hook are logged and fall back — never break the viewer.
- **Done when:** every entity in both worlds shows its in-game sprite (static first frame) or a labelled
  fallback; Play afterwards works and the pool-leak asserts stay quiet.
- *As built:* no separate Wren boot was needed - `StartGameSystems` already registers every GameObject script.
  `WrenScriptComponent::previewMethodHandle` is only created when `Class.type` has `preview(_,_)`.
  Placeholders are drawn by `DebugSystem` for any `Component::LevelEntityInstance` without a Sprite - no hook,
  *or* a hook that added no Sprite (e.g. `Item` "Safe"). Hooks exist on Chicken, Chinlin, NPC
  and Item - the only types placed in the LDtk files; Door/Sign/PressurePlate are spawned by states, so they
  can get one if they're ever placed in LDtk.

### Phase 4 — Viewer UI & overlays
- Level list: click = select, double-click = frame, eye toggle = hide/show (`Inactive`).
- Layer toggles by identifier (IntGrid/`Collision` off by default), entities on/off.
- Overlays via `DebugRenderer`: level bounds + identifiers, neighbour links, entity pivot markers.
- Hover/click picking of entities (AABB from sprite or LDtk size) → Inspector shows identifier, Iid,
  LDtk field values (already shown for level/world components in `InspectorWindow`).
- Status line: mouse world px, grid cell, room under cursor.
- **Done when:** you can answer "what is this, which room, what are its fields" without opening LDtk.
- *As built:* every previewed entity carries a `LevelEntityInstance` (identifier, Iid, script args, LDtk rect,
  colour) - used for picking (smallest LDtk rect under the cursor), the Inspector's "LDtk Entity" section and
  placeholders. Overlays are drawn with ImGui into the Game Viewport's draw list (`RenderViewportOverlay`), not
  `DebugRenderer`, so they can have text. All layers start visible (the `Collision` IntGrid has no tiles, so
  there is nothing to hide by default). Visibility is per session, levels keyed by Iid, layers by identifier.
  Picking uses the LDtk rect, not the sprite, so it matches what LDtk shows.

### Phase 5 — Quality of life (pick as needed)
- **Hot reload:** poll the `.ldtk` mtime; on change re-open, preserving camera and toggles (great with LDtk side-by-side).
- **Animate previews:** toggle that ticks only `AnimationSystem` in LevelView.
- **Play from here:** start the game with the player placed at the clicked room/position (needs a small
  state-side hook in `GameWorldState`/`ExperimentState` to read a "debug spawn" override).
- **Visible-room streaming:** load levels intersecting the view rect + margin, unload others via
  `UnloadLevelEntities` — only worth it once worlds get large.
- Web-editor build check (Wren isn't started there before Play — the Phase 3 minimal boot covers it).
- *As built:* hot reload, animated previews and Play from here are done; streaming and the web check are not.
  - Hot reload polls `FileSystem::GetModificationTime` every 0.5 s and reloads only once the time has held for
    a poll *and* the file parses (`nlohmann::json::accept`) - LevelParser asserts/throws on a half-written save.
  - Animate ticks `AnimationSystem` alone. Chicken/Chinlin share `static createAnimations(entity)` between
    `start()` and `preview`, which plays an idle clip; NPC sheets already had their animations.
  - Play from here: Ctrl+click in the viewport sets `GameData::debugSpawnWorldFile/Position`, read in Wren as
    `Application.debugSpawn`. Only `ExperimentState` (SevenGods) uses it - `GameWorldState` (MemoryPalace) places
    its rooms at hand-coded positions, not their LDtk ones, so a viewer position means nothing there. Plain Play
    clears it; Refresh keeps it.

---

## Risks / things to verify early
- **Wren VM lifetime:** viewer boot and Play's `StartDebugGame` must not overlap; the viewer's VM is shut
  down by `ClearGameSystems` like the game's. Remember the reused-state UI teardown rule if any viewer
  code ends up in Wren state singletons.
- **Resource pool asserts** in `ClearGameSystems`: preview textures are game-pool resources; they're freed
  by `registry.clear()`/VM shutdown — confirm with Play→Stop→View→Play cycles.
- **Draw order:** entities use `RenderLayer.ENTITIES` with orderInLayer from scripts; tile layers use the
  parser's `orderInLayer`. Should "just work", but y-sorting (if any) happens in an Update system — check.
- **Entity positions:** loader offsets by `gridSize/2`; fallback rects should use LDtk pivot/size so they
  line up with what LDtk shows.
