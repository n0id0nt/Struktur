#include "App.h"

#include <string>

#ifdef PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
#endif

#include <SDL3/SDL.h>

#include "Debug/Assertions.h"
#include "Debug/Profiling/Profiler.h"
#include "Engine/Callback/Variant.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/Core/EngineSettings.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/ECS/WrenScriptComponentRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Input/Input.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"
#include "Engine/Text/Font.h"
#include "Engine/UI/InputGlyphs.h"
#include "Engine/UI/UIManager.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"
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
#ifdef EDITOR
	#include <imgui.h>
	#include <imgui_impl_sdl3.h>

	#include "Debug/Editor/Editor.h"
	#include "Debug/Editor/EditorLayoutManager.h"
	#include "Debug/Editor/EditorTheme.h"
	#include "Debug/Editor/SaveFolders.h"
	#include "Engine/Renderer/ImGuiRenderer.h"
#endif

namespace Struktur::App
{
#ifdef PLATFORM_WEB
// The browser's loop calls a plain function, so the game's frame function is kept here.
static FrameFunction s_webFrame = nullptr;

static void WebFrame(void* context)
{
	s_webFrame(*static_cast<GameContext*>(context));
}
#endif

void MountAssets()
{
	FileSystem::Init(FileSystem::GetWorkingDirectory());
#if defined(PLATFORM_WEB)
	// Emscripten preloads assets into its virtual FS
	// Mount the preloaded assets directory directly
	FileSystem::Mount("/assets");
#elif defined(EDITOR)
	// Mounted by OpenEditor, from the game's asset folder.
#elif defined(DEBUG)
	FileSystem::Mount("assets/");
#else
	FileSystem::Mount("data.pak");
#endif
}

#ifdef EDITOR
void OpenEditor(GameContext& context, AddViewsFunction addGameViews)
{
	// A larger window than the game's, to fit the editor's panels.
	const int windowWidth    = 1280;
	const int windowHeight   = 720;
	Core::GameData& gameData = context.GetGameData();
	gameData.gameWidth       = windowWidth;
	gameData.gameHeight      = windowHeight;

	Platform::Window& window = context.GetWindow();
	window.Initialise(windowWidth, windowHeight, "Struktur", true);
	Renderer::GraphicsDevice& graphicsDevice = context.GetGraphicsDevice();
	graphicsDevice.Initialise(window.GetNativeHandle(), windowWidth, windowHeight);

	// Views draw in the order they're added: the game's, debug drawing over them, the UI, debug highlights over the
	// UI, then the editor.
	Debug::DebugViews debugViews;
	addGameViews(context);
	debugViews.world = graphicsDevice.AddView(Renderer::GraphicsDevice::ViewTarget::Game);
	context.GetUIRenderer().Initialise(graphicsDevice);
	debugViews.screen = graphicsDevice.AddView(Renderer::GraphicsDevice::ViewTarget::Game);

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
	context.GetImGuiRenderer().Initialise(graphicsDevice);

	// Set before the first ImGui frame - ImGui reads its ini once, on the first NewFrame, so the editor's docking
	// layout has to be the file loaded then (Editor::Initialise sets the same path again).
	io.IniFilename = Debug::EditorLayoutManager::kDefaultLayoutFilePath;
	Debug::EditorTheme::ApplyDarkTheme();

	FileSystem::Mount(STRUKTUR_ASSETS_DIR);

	Debug::Editor& editor = context.GetEditor();
	editor.SetDebugViews(debugViews);
	editor.Initialise(context);
}
#endif

void LoadScripts(GameContext& context)
{
	DEBUG_INFO("[Start] Wren Script Engine");
	context.GetWrenScriptEngine().Initialise(context);

	DEBUG_INFO("[Start] Wren State Manager");
	context.GetWrenStateManager().Initialise(context);

	DEBUG_INFO("[Start] Wren Script Component Registry");
	context.GetWrenScriptComponentRegistry().LoadAllScriptComponents(context);
}

#ifndef EDITOR
void OpenWindow(GameContext& context, AddViewsFunction addGameViews)
{
	// Outside the editor the window matches the game's size.
	Core::GameData& gameData = context.GetGameData();
	Platform::Window& window = context.GetWindow();
	window.Initialise(gameData.gameWidth, gameData.gameHeight, gameData.projectName, false);
	Renderer::GraphicsDevice& graphicsDevice = context.GetGraphicsDevice();
	graphicsDevice.Initialise(window.GetNativeHandle(), gameData.gameWidth, gameData.gameHeight);
	// The game's views first, so the UI draws over them.
	addGameViews(context);
	context.GetUIRenderer().Initialise(graphicsDevice);
	if (gameData.isFullScreen)
	{
		window.SetFullscreen(true);
	}
}
#endif

void ForwardWindowEvents(GameContext& context)
{
	// Touch-finger events reach Input the same way ImGui's own events do (Window::AddEventCallback) - touch is real
	// gameplay input, not an editor-only concern. Mouse/keyboard/gamepad stay polled in Input::Update(); only touch
	// needs the raw event stream (see Input::HandleEvent's own comment).
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
}

std::string MountSaves(GameContext& context)
{
#if defined(PLATFORM_WEB)
	std::string saveDir = "/saves";  // Emscripten virtual path
#elif defined(EDITOR)
	// <saves folder>/Current - the saves folder is picked in the editor's Saves window (default <assets>/../saves)
	std::string saveDir = Debug::SaveFolders::Initialise(STRUKTUR_ASSETS_DIR);
#else
	std::string saveDir = FileSystem::GetSaveDir("StrukturGames", context.GetGameData().projectName);
#endif
	FileSystem::SetWriteDir(saveDir);
	FileSystem::Mount(saveDir, "/", false);
	return saveDir;
}

void ApplyEngineSettings(GameContext& context)
{
	// GraphicsDevice::Initialise already ran with the VSync-on default, since it happens before the save folder that
	// holds the player's choice is mounted - applying the real value here (a bgfx::reset, via SetVSync) is still
	// before the first frame is presented.
	Core::GameData& gameData            = context.GetGameData();
	Core::EngineSettings engineSettings = Core::EngineSettings::Load();
	context.GetGraphicsDevice().SetVSync(engineSettings.vsync);
	gameData.vsyncEnabled = engineSettings.vsync;
	gameData.targetFps    = engineSettings.targetFps;
}

void StartPhysics(GameContext& context)
{
#ifdef STRUKTUR_PHYSICS
	DEBUG_INFO("[Start] Physics World");
	Core::GameData& gameData = context.GetGameData();
	glm::vec2 gravity(0.0f, 0.0f);
	context.GetPhysicsWorld().Initialise(gravity, gameData.velocityIterations, gameData.positionIterations,
	                                     gameData.pixelsPerMeter);
#endif
}

void BeginFrame(GameContext& context)
{
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
}

void SplashScreenFrame(GameContext& context, const SplashScreen& splash)
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

	std::string splashScreenName = splash.text;
	int fontSize                 = 120;

	Resource::ResourcePtr<Resource::FontResource> font =
	    resourceManager.GetFont(context, splash.font, splash.fontSize, splash.pixelFont);
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

	// UIRenderer has no immediate-mode draw path (see its class comment) - the splash screen runs outside the normal
	// UIRenderSystem pipeline (SPLASH_SCREEN never reaches systemManager.Update/Render), so it owns a batch and calls
	// Flush() itself instead, the same way UIManager's focus indicator owns its own batch outside any one UIElement.
	// Created once and reused every frame rather than every call - text content never changes, only its color (the
	// fade), which still needs a fresh DrawText each frame regardless.
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
	// In EDITOR builds, GameViewportWindow::Initialise already redirected the game's views into the editor's
	// offscreen game-viewport framebuffer before this ever runs - drawing there would be invisible since nothing
	// composites that texture onto the real window until editor.Update() runs, which the splash screen never calls.
	// Bypass the redirect so this frame's UI draws land on the real backbuffer instead - both calls are safe no-ops
	// on builds that never redirected in the first place (no editor, or web).
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

void PlayFrame(GameContext& context)
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

void StopLoop(GameContext& context)
{
#ifdef PLATFORM_WEB
	// Unwinds emscripten_set_main_loop_arg's simulated infinite loop, so control falls through to the game's cleanup.
	emscripten_cancel_main_loop();
#else
	context.GetGameData().gameState = Core::GameState::QUIT;
#endif
}

void EndFrame(GameContext& context)
{
	// The frame is submitted: post-frame jobs (PostFrame) start now and overlap the next frame's input/update.
	context.GetFrameScheduler().EndFrame();
	PROFILE_END_FRAME();
}

void RunLoop(GameContext& context, FrameFunction frame)
{
#ifdef PLATFORM_WEB
	// Web platform - use emscripten main loop
	s_webFrame = frame;
	emscripten_set_main_loop_arg(WebFrame, &context, 0, 1);
#else
	// Desktop platform - standard game loop (frame pacing comes from GraphicsDevice's BGFX_RESET_VSYNC and,
	// when gameData.targetFps > 0, the cap below - see Core::EngineSettings/SettingsWindow)
	Core::GameData& gameData = context.GetGameData();
	while (gameData.gameState != Core::GameState::QUIT)
	{
		uint64_t frameStartTicks = SDL_GetPerformanceCounter();

		frame(context);

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
				elapsedSeconds =
				    static_cast<double>(SDL_GetPerformanceCounter() - frameStartTicks) / static_cast<double>(frequency);
			}
		}
	}
#endif
}

void ResetPlayState(GameContext& context)
{
	// So a restart doesn't fire a burst of stale catch-up fixed steps carried over from before it.
	context.GetGameData().physicsAccumulator = 0.0f;

#ifdef STRUKTUR_FLAG
	DEBUG_INFO("[Clean Up] FlagManager");
	context.GetFlagManager().Clear();
#endif

	DEBUG_INFO("[Clean Up] Input");
	context.GetInput().Clear();

	// The button-hint art cache holds ResourcePtrs to game textures (loaded the first time a hint is shown), so it
	// has to let go of them before the resource pools are checked.
	DEBUG_INFO("[Clean Up] Input Glyphs");
	context.GetInputGlyphs().Clear();
}

void ClearScriptWorld(GameContext& context)
{
	DEBUG_INFO("[Clean Up] Wren State Manager");
	context.GetWrenStateManager().Shutdown(context);

	DEBUG_INFO("[Clean Up] UI Manager");
	context.GetUIManager().Clear(context);

	DEBUG_INFO("[Clean Up] Registry");
	context.GetRegistry().clear();
}

void ClearScriptServices(GameContext& context)
{
#ifdef STRUKTUR_PHYSICS
	DEBUG_INFO("[Clean Up] Physics World");
	context.GetPhysicsWorld().Clear();
#endif

#ifdef STRUKTUR_DIALOGUE
	DEBUG_INFO("[Clean Up] Dialogue Registry");
	context.GetDialogueRegistry().Clear(context);
#endif

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	context.GetWrenScriptComponentRegistry().Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	context.GetWrenScriptEngine().Shutdown();

#ifdef STRUKTUR_DIALOGUE
	DEBUG_INFO("[Clean Up] Dialogue Manager");
	context.GetDialogueManager().Clear();
#endif
}

void ReleaseGameResources(GameContext& context)
{
	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();

	// Everything cleared before this (UI Manager, Wren VM shutdown's finalizers, etc.) should have released every
	// ResourcePtr it was holding - if a game pool still shows resources loaded here, something is leaking a
	// reference instead of letting it go out of scope/be explicitly unloaded. Assert now, while it's still
	// attributable to a specific pool, rather than letting ClearGameResources() below silently force-free it out
	// from under whatever's still holding it. Only the game pools are checked/cleared: the editor holds its own
	// ResourcePtrs (thumbnails, previews) in the separate editor pools via ResourceManager::GetEditor*(), and those
	// are meant to survive a game reset untouched.
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

void CloseGame(GameContext& context)
{
	DEBUG_INFO("[Clean Up] Resource Manager");
	context.GetResourceManager().Clear();

#ifdef EDITOR
	DEBUG_INFO("[Clean Up] Editor");
	context.GetEditor().Shutdown(context);
#endif

	FileSystem::Shutdown();
}

void Shutdown(GameContext& context)
{
#ifdef EDITOR
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
#endif
	context.Shutdown();
}
}  // namespace Struktur::App
