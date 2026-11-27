#include "Game.h"

#include <cstdio>
#include <memory>
#include <string>
#include <variant>

#ifdef PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
#endif

#include "Debug/Assertions.h"
#include "Debug/Profiling/Profiler.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/Core/EngineSettings.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/System/CameraSystem.h"
#include "Engine/ECS/System/EventSystem.h"
#include "Engine/ECS/System/FitnessSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/PacerSystem.h"
#include "Engine/ECS/System/RideSystem.h"
#include "Engine/ECS/System/RiderAnimationSystem.h"
#include "Engine/ECS/System/SoundSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/UIRenderSystem.h"
#include "Engine/ECS/System/UIsystem.h"
#include "Engine/ECS/System/WorldRenderSystem.h"
#include "Engine/ECS/System/WorldSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/System/WrenStateSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/ECS/WrenScriptComponentRegistry.h"
#include "Engine/Fitness/FitnessManager.h"
#include "Engine/GameContext.h"
#include "Engine/Input/Input.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/RideIntegration/RideContext.h"
#include "Engine/RideIntegration/RideService.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"
#include "Engine/Text/Font.h"
#include "Engine/UI/InputGlyphs.h"
#include "Engine/UI/UIManager.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"
#include "Engine/World/World.h"
#include "Engine/World/WorldAssetLoader.h"
#include "Engine/World/WorldLifecycle.h"
#include "entt/entt.hpp"
#ifdef STRUKTUR_PHYSICS
	#include "Engine/ECS/System/PhysicsSystem.h"
	#include "Engine/Physics/PhysicsWorld.h"
#endif
#ifdef STRUKTUR_DIALOGUE
	#include "Engine/Dialogue/DialogueManager.h"
	#include "Engine/Dialogue/DialogueRegistry.h"
#endif
#ifdef STRUKTUR_FLAG
	#include "Engine/Flag/FlagManager.h"
#endif

#include <SDL3/SDL.h>

#ifdef EDITOR
	#include <imgui.h>
	#include <imgui_impl_sdl3.h>

	#include "Debug/Editor/DebugSystem.h"
	#include "Debug/Editor/Editor.h"
	#include "Debug/Editor/EditorLayoutManager.h"
	#include "Debug/Editor/EditorTheme.h"
	#include "Debug/Editor/ProjectLauncher.h"
	#include "Debug/Editor/SaveFolders.h"
	#include "Engine/Renderer/ImGuiRenderer.h"
#endif

// Encodes this game's saves (FileSystem::SetSaveKey). Keep it: changing it makes existing saves unreadable.
static constexpr uint8_t k_saveKey[] = {0x4B, 0x72, 0x69, 0x63, 0x6B, 0x65, 0x74, 0x47,
                                        0x61, 0x6D, 0x65, 0x53, 0x61, 0x76, 0x65, 0x31};

#ifdef EDITOR
	#define SPLASHSCREENFONT "Fonts/machine-std/machine-std-regular.ttf"
	#define SPLASHSCREENFONTSIZE 120
	#define SPLASHSCREENPIXELFONT false
	#define SPLASHSCREENTEXT "STRUKTUR"
#else
	// The game's pixel font (assets/Scripts/UI/Theme/UIKit.wren's font) at its 9px design size, sampled
	// nearest-neighbour - drawn at 120 it snaps to the nearest whole multiple (see Text::GlyphScale).
	#define SPLASHSCREENFONT "UI/Font/NormalFont.ttf"
	#define SPLASHSCREENFONTSIZE 9
	#define SPLASHSCREENPIXELFONT true
	#define SPLASHSCREENTEXT "Seven Gods"
#endif

bool Struktur::InitialiseGame(GameContext& context)
{
	Core::GameData& gameData                                       = context.GetGameData();
	System::SystemManager& systemManager                           = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager                   = context.GetGameObjectManager();
	Wren::WrenScriptEngine& wrenScriptEngine                       = context.GetWrenScriptEngine();
	Wren::WrenStateManager& wrenStateManager                       = context.GetWrenStateManager();
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();

	FileSystem::Init(FileSystem::GetWorkingDirectory());
	FileSystem::SetSaveKey({std::begin(k_saveKey), std::end(k_saveKey)});
#if defined(PLATFORM_WEB)
	// Emscripten preloads assets into its virtual FS
	// Mount the preloaded assets directory directly
	FileSystem::Mount("/assets");
#elif defined(EDITOR)
	// Mounted below, once the launcher (drawn in the editor window, which needs creating first) picks a project
	std::string projectPath;
#elif defined(DEBUG)
	FileSystem::Mount("assets/");
#else
	FileSystem::Mount("data.pak");
#endif

	// Want to create a window before we start initialising systems
#ifdef EDITOR
	// In debug mode, create a larger window to fit ImGui panels
	const int windowWidth  = 1280;
	const int windowHeight = 720;
	gameData.gameWidth     = windowWidth;
	gameData.gameHeight    = windowHeight;

	Platform::Window& window = context.GetWindow();
	window.Initialise(windowWidth, windowHeight, "Struktur", true);
	context.GetGraphicsDevice().Initialise(window.GetNativeHandle(), windowWidth, windowHeight);
	context.GetUIRenderer().Initialise();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// "ForOther" variant: handles input/platform (mouse, keyboard, clipboard, cursors) but expects a separate
	// renderer backend, which is our own hand-rolled bgfx one (Engine/Renderer/ImGuiRenderer) rather than an
	// official imgui_impl_* target.
	ImGui_ImplSDL3_InitForOther(window.GetSDLWindow());
	// Window::PollEvents() drains the SDL event queue itself and only exposes close/resize - this callback is
	// how ImGui sees the raw events it needs (text input, wheel, focus) that polled state can't provide.
	window.AddEventCallback([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });
	context.GetImGuiRenderer().Initialise();

	// Set before the launcher's first ImGui frame - ImGui reads its ini once, on the first NewFrame, so the
	// editor's docking layout has to be the file loaded then (Editor::Initialise sets the same path again).
	io.IniFilename = Debug::EditorLayoutManager::kDefaultLayoutFilePath;
	Debug::EditorTheme::ApplyDarkTheme();

	// Project folder selection at startup - an in-editor launcher of remembered projects
	Debug::ProjectLauncher projectLauncher;
	projectPath = projectLauncher.Run(context);
	if (projectPath.empty())
	{
		// Window closed in the launcher - nothing is mounted or initialised past this point
		return false;
	}
	FileSystem::Mount(projectPath);

	Debug::Editor& editor = context.GetEditor();
	editor.Initialise(context);
#endif

	wrenScriptEngine.Initialise(context);
	wrenStateManager.Initialise(context);
	wrenScriptComponentRegistry.LoadAllScriptComponents(context);

#ifndef EDITOR
	// In release mode, window matches game size
	Platform::Window& window = context.GetWindow();
	window.Initialise(gameData.gameWidth, gameData.gameHeight, gameData.projectName, false);
	context.GetGraphicsDevice().Initialise(window.GetNativeHandle(), gameData.gameWidth, gameData.gameHeight);
	context.GetUIRenderer().Initialise();
	if (gameData.isFullScreen)
	{
		window.SetFullscreen(true);
	}
#endif

	// Touch-finger events reach Input the same way ImGui's own events do above (Window::AddEventCallback) -
	// unconditional (not EDITOR-gated), since touch is real gameplay input, not an editor-only concern. Mouse/
	// keyboard/gamepad stay polled in Input::Update() as before; only touch needs the raw event stream (see
	// Input::HandleEvent's own comment).
	context.GetWindow().AddEventCallback([&context](const SDL_Event& event) { context.GetInput().HandleEvent(event); });
	// Files dropped on the window become a "fileDropped" engine event: {"path": String, UTF-8}.
	context.GetWindow().AddEventCallback(
	    [&context](const SDL_Event& event)
	    {
		    if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr)
		    {
			    Callback::VariantMap data;
			    data.items["path"] = std::string(event.drop.data);
			    context.GetEventManager().AddEvent("fileDropped", data);
		    }
	    });

#if defined(PLATFORM_WEB)
	std::string saveDir = "/saves";  // Emscripten virtual path
#elif defined(EDITOR)
	// <saves folder>/Current - the saves folder is picked in the editor's Saves window (default <project>/../saves)
	std::string saveDir = Debug::SaveFolders::Initialise(projectPath);
#else
	std::string saveDir = FileSystem::GetSaveDir("StrukturGames", gameData.projectName);
#endif
	FileSystem::SetWriteDir(saveDir);
	FileSystem::Mount(saveDir, "/", false);
	// Remember the player's trainer / HR strap / power meter next to the saves. Loading only reads the choices;
	// nothing connects until the game calls FitnessManager::ConnectRememberedDevices.
	context.GetFitnessManager().SetRegistryPath(saveDir + "/fitness_devices.json");
	context.GetFitnessManager().LoadRegistry();
	Struktur::RideContext::From(context).GetRideService().Initialise(saveDir);
	Struktur::RideContext::From(context).GetWorld().SetAssets(World::LoadWorldAssets());
	if (Core::CommandLine::HasFlag("--list-missing-assets"))
	{
		// Prints every ride-world art role still using a placeholder (Markdown, same table as docs/missing-assets.md)
		// and quits before the first frame.
		const World::AssetManifest& manifest = Struktur::RideContext::From(context).GetWorld().GetAssets().manifest;
		const std::string table              = World::GetMissingAssetsMarkdown(manifest);
		std::printf("%s", table.c_str());
		std::fflush(stdout);
		for (const std::string& line : World::GetMissingAssetLines(manifest))
		{
			DEBUG_INFO("missing %s", line.c_str());
		}
		gameData.gameState = Core::GameState::QUIT;
	}
	if (Core::CommandLine::HasFlag("--demo"))
	{
		// Demo: the simulated trainer and strap are "remembered" in memory only, so pairing connects them at once and
		// the player's real device choices on disk are never touched.
		Fitness::FitnessManager& fitness = context.GetFitnessManager();
		fitness.SetRegistryPath("");
		fitness.GetRegistry() = Fitness::DeviceRegistry();
		fitness.GetRegistry().Remember(Fitness::DeviceRole::TRAINER,
		                               Fitness::RememberedDevice{"5E:ED:00:00:00:01", "SIM Smart Trainer", "ble"});
		fitness.GetRegistry().Remember(Fitness::DeviceRole::HEART_RATE,
		                               Fitness::RememberedDevice{"5E:ED:00:00:00:02", "SIM Heart Rate", "ble"});
	}
	// Audio is set up by GameContext's constructor (SDL3_mixer), so it's already available here.

	// GraphicsDevice::Initialise (above) already ran with the hardcoded VSync-on default, since it happens
	// before the save-directory mount above even exists to read a persisted choice from - applying the real
	// value here (a bgfx::reset, via SetVSync) is still before the first frame is ever presented, so this is
	// imperceptible.
	Core::EngineSettings engineSettings = Core::EngineSettings::Load();
	context.GetGraphicsDevice().SetVSync(engineSettings.vsync);
	gameData.vsyncEnabled = engineSettings.vsync;
	gameData.targetFps    = engineSettings.targetFps;

	gameObjectManager.CreateObjectCallBack(context);

#ifdef STRUKTUR_PHYSICS
	glm::vec2 gravity(0.0f, 0.0f);
	context.GetPhysicsWorld().Initialise(gravity, gameData.velocityIterations, gameData.positionIterations,
	                                     gameData.pixelsPerMeter);
#endif

	// need a way to have helper systems with out an empty update
	systemManager.AddHelperSystem<System::HierarchySystem>();
	systemManager.AddHelperSystem<System::TransformSystem>();
	systemManager.AddUpdateSystem<System::WrenStateSystem>();
	systemManager.AddFixedUpdateSystem<System::WrenStateFixedUpdateSystem>();
	systemManager.AddFixedUpdateSystem<System::WrenScriptFixedUpdateSystem>();
#ifdef STRUKTUR_PHYSICS
	// Fixed-cadence Box2D step only - kept separate from PhysicsSystem's own per-frame Update (registered below,
	// right before CameraSystem, same relative slot it always had) which handles the Transform<->physics sync.
	// See PhysicsFixedStepSystem's own comment for why these two can't both move to fixed-rate together.
	systemManager.AddFixedUpdateSystem<System::PhysicsFixedStepSystem>();
#endif
	// Before WrenScriptSystem/EventSystem so fitness events queued this frame are delivered this frame.
	systemManager.AddUpdateSystem<System::FitnessSystem>();
	systemManager.AddUpdateSystem<System::RideSystem>();
	// The world: advance the road and the player, then move what rides on it, then the camera that follows.
	systemManager.AddUpdateSystem<System::WorldSystem>();
	systemManager.AddUpdateSystem<System::PacerSystem>();
	systemManager.AddUpdateSystem<System::RiderAnimationSystem>();
	systemManager.AddUpdateSystem<System::CameraSystem>();
	systemManager.AddUpdateSystem<System::WrenScriptSystem>();
	systemManager.AddUpdateSystem<System::EventSystem>();
#ifdef STRUKTUR_PHYSICS
	systemManager.AddUpdateSystem<System::PhysicsSystem>();
#endif
	systemManager.AddUpdateSystem<System::UISystem>();
	systemManager.AddUpdateSystem<System::SoundSystem>();
	// The world has its own bgfx views, ordered before the UI's (see GraphicsDevice).
	systemManager.AddRenderSystem<System::WorldRenderSystem>();
	// Wren rendering not used for anything at the moment so commenting it out
// #if defined(PLATFORM_WEB)
//	systemManager.AddRenderSystem<System::WrenStateRenderSystem>();
// #endif
#ifdef EDITOR
	systemManager.AddRenderSystem<System::DebugSystem>();
#endif
	systemManager.AddRenderSystem<System::UIRenderSystem>();
	// Worker-safe systems (SystemAccess) run side by side on the job system; this runs them all on the main thread.
	if (Core::CommandLine::HasFlag("--serial-systems"))
	{
		systemManager.SetParallel(false);
		DEBUG_INFO("--serial-systems - every system runs on the main thread");
	}

	DEBUG_INFO("Game Data Loaded");

	// Release builds always start Wren immediately. Web-editor builds wait for the toolbar's Play button
	// (StartDebugGame - a full ClearGameSystems + StartGameSystems + Start() restart) before Wren runs at all.
	// Desktop-editor builds start Wren once here too, purely so the initial scene is loaded and visible in the
	// Game Viewport before Play is first pressed - GameLoop's Update() stays gated on debugSettings.playingGame
	// on both platforms either way, so nothing actually simulates until Play/Refresh is clicked.
#if !defined(EDITOR)
	wrenStateManager.Start(context);
#endif
	return true;
}

void Struktur::ExitGame(GameContext& context)
{
	DEBUG_INFO("[Clean Up] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Shutdown(context);

	DEBUG_INFO("[Clean Up] UI Manager");
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Clear(context);

	DEBUG_INFO("[Clean Up] Registry");
	entt::registry& registry = context.GetRegistry();
	registry.clear();

	DEBUG_INFO("[Clean Up] Game Object Manager");
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	gameObjectManager.CreateObjectCallBack(context);

#ifdef STRUKTUR_PHYSICS
	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();
#endif

#ifdef STRUKTUR_DIALOGUE
	DEBUG_INFO("[Clean Up] Dialogue Registry");
	Dialogue::DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
	dialogueRegistry.Clear(context);
#endif

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

#ifdef STRUKTUR_DIALOGUE
	DEBUG_INFO("[Clean Up] DialogueManager");
	Dialogue::DialogueManager& dialogueManager = context.GetDialogueManager();
	dialogueManager.Clear();
#endif

	// Before the resource manager: the world renderer holds textures.
	DEBUG_INFO("[Clean Up] World");
	World::UnloadWorld(context);

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	resourceManager.Clear();

#ifdef EDITOR
	DEBUG_INFO("[Clean Up] Editor");
	Debug::Editor& editor = context.GetEditor();
	editor.Shutdown(context);
#endif

	FileSystem::Shutdown();
}

void Struktur::SplashScreenLoop(GameContext& context)
{
	Core::GameData& gameData                   = context.GetGameData();
	Core::TimeSystem& timeSystem               = context.GetTimeSystem();
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	// fade in time
	const double fadeInTime  = 1.5;
	const double holdTime    = 1;
	const double fadeOutTime = 1.5;
	const double currentTime = timeSystem.unscaledTime;
	const double startTime   = 0;
	if (currentTime > startTime + fadeInTime + holdTime + fadeOutTime)
	{
		gameData.gameState = Core::GameState::GAME;
		DEBUG_INFO("Start Game Loop");
	}

	double textAlpha = 255;
	// Fade in
	if (currentTime < startTime + fadeInTime)
	{
		float t = (currentTime - startTime) / fadeInTime;
		textAlpha *= Util::Math::Lerp(0.f, 1.f, t);
	}
	// Fade out
	else if (currentTime > startTime + fadeInTime + holdTime &&
	         currentTime < startTime + fadeInTime + holdTime + fadeOutTime)
	{
		float t = (currentTime - startTime - fadeInTime - holdTime) / fadeOutTime;
		textAlpha *= Util::Math::Lerp(1.f, 0.f, t);
	}

	std::string splashScreenName = SPLASHSCREENTEXT;
	int fontSize                 = 120;

	Resource::ResourcePtr<Resource::FontResource> font =
	    resourceManager.GetFont(context, SPLASHSCREENFONT, SPLASHSCREENFONTSIZE, SPLASHSCREENPIXELFONT);
	// Pinned once (see ResourcePool<T>::Pin) instead of holding a persistent ResourcePtr somewhere for the whole
	// splash duration - without this, GetResource()'s cache would evict/reload the font every single frame, since
	// `font` above is the only reference to it and it goes out of scope at the end of this function. Pinning lets
	// this stay a plain per-frame re-fetch with nothing to own long-term.
	bool fontPinned = font.IsPinned();
	if (!fontPinned)
	{
		font.Pin();
		fontPinned = true;
	}
	if (!font->IsGpuReady())
	{
		font->LoadToGpu(context);
	}
	int fontWidth = (int)Text::MeasureTextEx(font->font, splashScreenName, (float)fontSize, 1.0f).x;
	// Centered in "game" pixel space (gameWidth/gameHeight), not the real window's applicationWidth/Height -
	// UIRenderer::SetupView's orthographic projection is fixed to the former (see UILabel/DebugSystem for the
	// same convention), so that's the space any screen-space text needs to be positioned in here too.
	int width  = gameData.gameWidth;
	int height = gameData.gameHeight;

	// UIRenderer has no immediate-mode draw path (see its class comment) - this loop runs outside the normal
	// GameLoop/UIRenderSystem pipeline (SPLASH_SCREEN never reaches systemManager.Update/Render), so it owns a
	// batch and calls Flush() itself instead, the same way UIManager's focus indicator owns its own batch outside
	// any one UIElement. Created once and reused every frame rather than every call - text content never
	// changes, only its color (the fade), which still needs a fresh DrawText each frame regardless.
	static Renderer::UIBatchHandle splashBatch;
	static Renderer::UIBatchSlot splashSlot;
	if (!splashBatch.IsValid())
	{
		splashBatch = context.GetUIRenderer().CreateBatch();
		// splashScreenName.size() (raw byte length) is a safe over-estimate of its codepoint count (DrawText's
		// own required-capacity contract) - fine here since, unlike UILabel, nothing else shares this slot that
		// would need an exact fit.
		splashSlot =
		    context.GetUIRenderer().AllocateOrResizeSlot(splashBatch, splashSlot, (uint32_t)splashScreenName.size());
	}

	context.GetGraphicsDevice().BeginFrame();
	// In EDITOR builds, GameViewportWindow::Initialise already redirected World/Debug/UI views into the
	// editor's offscreen game-viewport framebuffer before this loop ever runs - drawing there would be
	// invisible since nothing composites that texture onto the real window until editor.Update() runs, which
	// this splash-only loop never calls. Bypass the redirect so this frame's UI draws land on the real
	// backbuffer instead - both calls are safe no-ops on builds that never redirected in the first place (no
	// editor, or web).
	context.GetGraphicsDevice().ResetWorldRenderTarget();
	context.GetUIRenderer().SetupView(context);
	context.GetUIRenderer().DrawText(splashBatch, splashSlot, font->font, splashScreenName,
	                                 {(float)((width - fontWidth) / 2), (float)((height - fontSize) / 2)},
	                                 (float)fontSize, Util::Color{255, 255, 255, (unsigned char)textAlpha}, 0);
	context.GetUIRenderer().Flush(context);
	context.GetGraphicsDevice().EndFrame();

	if (gameData.gameState == Core::GameState::GAME)
	{
		font.Unpin();
		context.GetGraphicsDevice().RestoreWorldRenderTarget();
		context.GetUIRenderer().DestroyBatch(splashBatch);
		splashBatch = Renderer::UIBatchHandle{};
		splashSlot  = Renderer::UIBatchSlot{};
	}
}

void Struktur::GameLoop(GameContext& context)
{
	PROFILE_BEGIN_SCOPE(gameLoop, "Game Loop");
	System::SystemManager& systemManager         = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	{
		PROFILE_SCOPE("Update Processing");
#ifdef EDITOR
		auto& debugSettings = context.GetEditor().GetSettings().debugRender;
		if (debugSettings.playingGame && !debugSettings.pausedGame)
		{
#endif
			Core::GameData& gameData = context.GetGameData();
			gameData.physicsAccumulator += context.GetTimeSystem().scaledDelta;
			{
				// Kept as its own scope (always entered, even on a zero-step frame) so the profiler shows the
				// fixed-rate simulation as a distinct sibling of "Update" / "Render" - call count on this node
				// is how many Box2D steps this frame ran.
				PROFILE_SCOPE("Fixed Update");
				int fixedSteps = 0;
				while (gameData.physicsAccumulator >= gameData.timeStep && fixedSteps < gameData.maxFixedStepsPerFrame)
				{
#ifdef STRUKTUR_PHYSICS
					// Once per STEP, right before it runs - captures where bodies sit now as the blend-from point
					// for the step about to advance them (see PhysicsSystem::SyncPhysicsToTransforms). This has to
					// be per-step, not once per frame: the render-time blend only ever spans a single timeStep
					// (alpha = accumulator / timeStep), so previous must always sit exactly one step behind current.
					// Snapshotting only before the first step of a multi-step frame leaves previous N-1 steps stale,
					// and the motion of every step but the last then lands instantly and uninterpolated at the top
					// of the frame - visible jitter whenever the frame rate drops below the fixed rate. A zero-step
					// frame still never calls this (the loop body doesn't run), so the blend keeps smoothly
					// approaching an unchanged "current" as the accumulator grows toward the next step.
					systemManager.GetSystem<System::PhysicsSystem>().SnapshotPreviousTransforms(context);
#endif
					systemManager.FixedUpdate(context);
					gameData.physicsAccumulator -= gameData.timeStep;
					fixedSteps++;
				}
				if (fixedSteps == gameData.maxFixedStepsPerFrame)
				{
					// Discard the overflow rather than chase it - running every queued step after a big stall
					// would itself cost real time, risking the same stall on the next frame too (the
					// spiral-of-death this cap exists to avoid). Physics simply falls behind briefly instead of
					// freezing.
					gameData.physicsAccumulator = 0.0f;
				}
			}

			{
				PROFILE_SCOPE("Update");
				systemManager.Update(context);
			}
#ifdef EDITOR
		}
#endif
		{
			// flush queues
			PROFILE_SCOPE("Queue Flush");
			gameObjectManager.UpdateGameObjectsActiveStateQueue(context);
			gameObjectManager.DeleteGameObjectsInSafeToDeleteQueue(context);
		}
	}

	context.GetGraphicsDevice().BeginFrame();
#ifdef EDITOR
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	Debug::Editor& editor = context.GetEditor();
#endif
	{
		PROFILE_SCOPE("Render");
		systemManager.Render(context);
	}
	PROFILE_END_SCOPE(gameLoop);
#ifdef EDITOR
	editor.Update(context);
#endif
	context.GetGraphicsDevice().EndFrame();
}

void Struktur::UpdateLoop(void* userData)
{
	GameContext& context = *static_cast<GameContext*>(userData);
	PROFILE_BEGIN_FRAME();

	Core::GameData& gameData     = context.GetGameData();
	Core::TimeSystem& timeSystem = context.GetTimeSystem();
	// Before PollEvents() - a touch event arriving during this frame's poll needs the window size to convert
	// its normalized (0-1) coordinates (see Input::HandleEvent/SetWindowSize's own comments); using the size as
	// of the end of last frame is close enough (a resize and a touch landing in the same poll batch is a rare,
	// harmless off-by-one-frame edge case, not worth guarding further).
	context.GetInput().SetWindowSize(context.GetWindow().GetWidth(), context.GetWindow().GetHeight());
	context.GetWindow().PollEvents();
	context.GetInput().Update();
	gameData.applicationWidth  = context.GetWindow().GetWidth();
	gameData.applicationHeight = context.GetWindow().GetHeight();
	if (context.GetWindow().WasResized())
	{
		context.GetGraphicsDevice().Resize(gameData.applicationWidth, gameData.applicationHeight);
	}
	timeSystem.Update();
	// Sync point: last frame's post-frame jobs are done, and finished tasks hand their results to the main thread.
	{
		PROFILE_SCOPE("Jobs Sync");
		context.GetFrameScheduler().BeginFrame();
	}

	if (context.GetWindow().ShouldClose())
	{
		gameData.gameState = Core::GameState::QUIT;
	}

	switch (gameData.gameState)
	{
		case Core::GameState::SPLASH_SCREEN:
			SplashScreenLoop(context);
			break;

		case Core::GameState::GAME:
			GameLoop(context);
			break;

		default:
#ifdef PLATFORM_WEB
			// Unwinds emscripten_set_main_loop_arg's simulated infinite loop, letting control fall through to
			// Game()'s single shared cleanup block (previously this branch also called ExitGame() itself,
			// double-running cleanup once Game() no longer has a platform-specific tail - fixed while unifying).
			emscripten_cancel_main_loop();
#else
			gameData.gameState = Core::GameState::QUIT;
#endif
			break;
	}

	// The frame is submitted: post-frame jobs (PostFrame) start now and overlap the next frame's input/update.
	context.GetFrameScheduler().EndFrame();
	PROFILE_END_FRAME();
}

void Struktur::Game()
{
	RideContext context;

	// Load resources
	if (!InitialiseGame(context))
	{
		// Closed from the editor's project launcher before any project was opened - only the window, graphics
		// device and ImGui exist yet, so skip ExitGame's game-side teardown.
#ifdef EDITOR
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
#endif
		context.Shutdown();
		FileSystem::Shutdown();
		return;
	}

	Core::GameData& gameData = context.GetGameData();

#ifdef PLATFORM_WEB
	// Web platform - use emscripten main loop
	emscripten_set_main_loop_arg(UpdateLoop, &context, 0, 1);
#else
	// Desktop platform - standard game loop (frame pacing comes from GraphicsDevice's BGFX_RESET_VSYNC and,
	// when gameData.targetFps > 0, the cap below - see Core::EngineSettings/SettingsWindow)
	while (gameData.gameState != Core::GameState::QUIT)
	{
		uint64_t frameStartTicks = SDL_GetPerformanceCounter();

		UpdateLoop(&context);

		// Effective frame budget: normally just the persisted targetFps cap, but the editor's Profiler window can
		// override it (preview a different framerate) and/or add a constant slice of artificial lag per frame.
		int effectiveTargetFps   = gameData.targetFps;
		double extraDelaySeconds = 0.0;
#if defined(EDITOR)
		if (gameData.frameRateOverrideEnabled)
		{
			effectiveTargetFps = gameData.overrideFps;
			extraDelaySeconds  = static_cast<double>(gameData.artificialLagMs) / 1000.0;
		}
		if (gameData.oneOffHitchMs > 0.0f)
		{
			// "Inject spike" button - one big hitch, then clear. Measured into the next frame's delta by
			// TimeSystem, so the game sees a real stall to recover from.
			SDL_Delay(static_cast<Uint32>(gameData.oneOffHitchMs));
			gameData.oneOffHitchMs = 0.0f;
		}
#endif

		double targetFrameSeconds = effectiveTargetFps > 0 ? 1.0 / static_cast<double>(effectiveTargetFps) : 0.0;
		targetFrameSeconds += extraDelaySeconds;

		if (targetFrameSeconds > 0.0)
		{
			uint64_t frequency = SDL_GetPerformanceFrequency();
			double elapsedSeconds =
			    static_cast<double>(SDL_GetPerformanceCounter() - frameStartTicks) / static_cast<double>(frequency);

			// Coarse-sleep most of the remaining budget - OS scheduler granularity (~1-15ms on Windows) is too
			// imprecise to sleep the whole remainder accurately - then spin-wait the last couple of ms for
			// precision.
			constexpr double kSpinWaitSeconds = 0.002;
			while (elapsedSeconds < targetFrameSeconds)
			{
				double remainingSeconds = targetFrameSeconds - elapsedSeconds;
				if (remainingSeconds > kSpinWaitSeconds)
				{
					SDL_Delay(static_cast<Uint32>((remainingSeconds - kSpinWaitSeconds) * 1000.0));
				}
				elapsedSeconds = static_cast<double>(SDL_GetPerformanceCounter() - frameStartTicks) /
				                 static_cast<double>(frequency);
			}
		}
	}
#endif

	// Cleanup
	ExitGame(context);
#ifdef EDITOR
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
#endif
	context.Shutdown();
}

#ifdef EDITOR
void Struktur::StartDebugGame(GameContext& context)
{
	ClearGameSystems(context);
	StartGameSystems(context);

	DEBUG_INFO("[Start] Wren State");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Start(context);
}

void Struktur::StopDebugGame(GameContext& context)
{
	ClearGameSystems(context);
}

void Struktur::ClearGameSystems(GameContext& context)
{
	// Reset so a restart doesn't fire a burst of stale catch-up fixed steps carried over from before the reset.
	context.GetGameData().physicsAccumulator = 0.0f;

	#ifdef STRUKTUR_FLAG
	DEBUG_INFO("[Clean Up] FlagManager");
	Flag::FlagManager& flagManager = context.GetFlagManager();
	flagManager.Clear();
	#endif

	DEBUG_INFO("[Clean Up] Input");
	Input::Input& input = context.GetInput();
	input.Clear();

	// The button-hint art cache holds ResourcePtrs to game textures (loaded the first time a hint is shown), so it
	// has to let go of them before the resource pools are checked below.
	DEBUG_INFO("[Clean Up] Input Glyphs");
	context.GetInputGlyphs().Clear();

	DEBUG_INFO("[Clean Up] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Shutdown(context);

	DEBUG_INFO("[Clean Up] UI Manager");
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Clear(context);

	DEBUG_INFO("[Clean Up] Registry");
	entt::registry& registry = context.GetRegistry();
	registry.clear();

	// A script loads the world; a restart starts without one.
	World::UnloadWorld(context);

	#ifdef STRUKTUR_PHYSICS
	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();
	#endif

	#ifdef STRUKTUR_DIALOGUE
	DEBUG_INFO("[Clean Up] Dialogue Registry");
	Dialogue::DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
	dialogueRegistry.Clear(context);
	#endif

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

	#ifdef STRUKTUR_DIALOGUE
	DEBUG_INFO("[Clean Up] Dialogue Manager");
	Dialogue::DialogueManager& dialogueManager = context.GetDialogueManager();
	dialogueManager.Clear();
	#endif

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();

	// Everything above (UI Manager, Wren VM shutdown's finalizers, etc.) should have already released every
	// ResourcePtr it was holding by this point - if a game pool still shows resources loaded
	// here, something is leaking a reference instead of letting it go out of scope/be explicitly unloaded. Assert
	// now, while it's still attributable to a specific pool, rather than letting ClearGameResources() below
	// silently force-free it out from under whatever's still holding it. Only the game pools are checked/cleared:
	// the editor holds its own ResourcePtrs (thumbnails, previews) in the separate editor pools via
	// ResourceManager::GetEditor*(), and those are meant to survive a game reset untouched.
	ASSERT_MSG(resourceManager.GetTexturePool().GetLoadedCount() == 0,
	           "Texture pool still has %zu resource(s) loaded before Clear() - something is leaking a ResourcePtr",
	           resourceManager.GetTexturePool().GetLoadedCount());
	ASSERT_MSG(resourceManager.GetSoundPool().GetLoadedCount() == 0,
	           "Sound pool still has %zu resource(s) loaded before Clear() - something is leaking a ResourcePtr",
	           resourceManager.GetSoundPool().GetLoadedCount());
	ASSERT_MSG(resourceManager.GetMusicPool().GetLoadedCount() == 0,
	           "Music pool still has %zu resource(s) loaded before Clear() - something is leaking a ResourcePtr",
	           resourceManager.GetMusicPool().GetLoadedCount());
	ASSERT_MSG(resourceManager.GetFontPool().GetLoadedCount() == 0,
	           "Font pool still has %zu resource(s) loaded before Clear() - something is leaking a ResourcePtr",
	           resourceManager.GetFontPool().GetLoadedCount());
	ASSERT_MSG(resourceManager.GetShaderPool().GetLoadedCount() == 0,
	           "Shader pool still has %zu resource(s) loaded before Clear() - something is leaking a ResourcePtr",
	           resourceManager.GetShaderPool().GetLoadedCount());

	resourceManager.ClearGameResources();
}

void Struktur::StartGameSystems(GameContext& context)
{
	#ifdef STRUKTUR_PHYSICS
	Core::GameData& gameData = context.GetGameData();
	#endif

	DEBUG_INFO("[Start] Wren State Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Initialise(context);

	DEBUG_INFO("[Start] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Initialise(context);

	DEBUG_INFO("[Start] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.LoadAllScriptComponents(context);

	#ifdef STRUKTUR_PHYSICS
	DEBUG_INFO("[Start] Physics World");
	glm::vec2 gravity(0.0f, 0.0f);
	context.GetPhysicsWorld().Initialise(gravity, gameData.velocityIterations, gameData.positionIterations,
	                                     gameData.pixelsPerMeter);
	#endif
}
#endif
