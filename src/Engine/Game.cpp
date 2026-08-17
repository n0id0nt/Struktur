#include "Game.h"

#include <memory>
#include <string>
#include <variant>

#ifdef PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
#endif

#include "Debug/Assertions.h"
#include "Debug/Profiling/Profiler.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/CameraSystem.h"
#include "Engine/ECS/System/DebugSystem.h"
#include "Engine/ECS/System/EventSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/SoundSystem.h"
#include "Engine/ECS/System/SpriteRenderSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/UIRenderSystem.h"
#include "Engine/ECS/System/UIsystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/System/WrenStateSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/FileLoading/LevelParser.h"
#include "Engine/GameContext.h"
#include "Engine/Input/Input.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Text/Font.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"
#include "Engine/World/Level.h"

#ifdef DEBUG
	#include "Engine/Scripting/WrenCodeGenerator.h"
#endif

#include "Engine/Platform/Window.h"
#include "Engine/Renderer/GraphicsDevice.h"
#ifdef EDITOR
	#include <imgui.h>
	#include <imgui_impl_sdl3.h>
#endif

#ifdef EDITOR
	#define SPLASHSCREENFONT "Fonts/medieval_sharp/MedievalSharp-Bold.ttf"
	#define SPLASHSCREENTEXT "STRUKTUR"
#else
	#define SPLASHSCREENFONT "Fonts/medieval_sharp/MedievalSharp-Bold.ttf"
	#define SPLASHSCREENTEXT "Memory Palace"
#endif

void Struktur::InitialiseGame(GameContext& context)
{
	Core::GameData& gameData                                       = context.GetGameData();
	System::SystemManager& systemManager                           = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager                   = context.GetGameObjectManager();
	Physics::PhysicsWorld& physicsWorld                            = context.GetPhysicsWorld();
	Wren::WrenScriptEngine& wrenScriptEngine                       = context.GetWrenScriptEngine();
	Wren::WrenStateManager& wrenStateManager                       = context.GetWrenStateManager();
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();

	FileSystem::Init(FileSystem::GetWorkingDirectory());
#if defined(PLATFORM_WEB)
	// Emscripten preloads assets into its virtual FS
	// Mount the preloaded assets directory directly
	FileSystem::Mount("/assets");
#elif defined(EDITOR)
	// Project folder selection at startup
	std::string projectPath =
	    FileSystem::OpenFolderDialog("Select Project Directory", FileSystem::GetWorkingDirectory());

	// Fall back to default assets/ if user cancels
	FileSystem::Mount(projectPath.empty() ? "assets/" : projectPath);
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
	window.SetEventCallback([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });
	context.GetImGuiRenderer().Initialise();

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

#if defined(PLATFORM_WEB)
	std::string saveDir = "/saves";  // Emscripten virtual path
#elif defined(EDITOR)
	std::string saveDir = (projectPath.empty() ? "" : projectPath) + "/../saves";
#else
	std::string saveDir = FileSystem::GetSaveDir("StrukturGames", gameData.projectName);
#endif
	FileSystem::SetWriteDir(saveDir);
	FileSystem::Mount(saveDir, "/", false);
	// Audio setup now happens in GameContext's constructor via SDL3_mixer, so it's already available here.

#ifdef DEBUG
	Wren::CodeGenerator::GenerateBindingFiles(wrenScriptEngine.GetRegistry(),
	                                          FileSystem::GetWorkingDirectory() + "/../src/WrenBindings/Bindings");
#endif

	gameObjectManager.CreateObjectCallBack(context);

	glm::vec2 gravity(0.0f, 0.0f);
	physicsWorld.Initialise(gravity, gameData.velocityIterations, gameData.positionIterations, gameData.pixelsPerMeter);

	// need a way to have helper systems with out an empty update
	systemManager.AddHelperSystem<System::HierarchySystem>();
	systemManager.AddHelperSystem<System::TransformSystem>();
	systemManager.AddHelperSystem<System::ShaderSystem>();
	systemManager.AddUpdateSystem<System::WrenStateSystem>();
	systemManager.AddUpdateSystem<System::WrenScriptSystem>();
	systemManager.AddUpdateSystem<System::EventSystem>();
	systemManager.AddUpdateSystem<System::PhysicsSystem>();
	systemManager.AddUpdateSystem<System::CameraSystem>();
	systemManager.AddUpdateSystem<System::AnimationSystem>();
	systemManager.AddUpdateSystem<System::UISystem>();
	systemManager.AddUpdateSystem<System::SoundSystem>();
	systemManager.AddRenderSystem<System::SpriteRenderSystem>();
	// Wren rendering not used for anything at the moment so commenting it out
// #if defined(PLATFORM_WEB)
//	systemManager.AddRenderSystem<System::WrenStateRenderSystem>();
// #endif
#ifdef EDITOR
	systemManager.AddRenderSystem<System::DebugSystem>();
#endif
	systemManager.AddRenderSystem<System::UIRenderSystem>();

	DEBUG_INFO("Game Data Loaded");

	// Release builds always start Wren immediately. Web-editor builds wait for the toolbar's Play button
	// (StartDebugGame - a full ClearGameSystems + StartGameSystems + Start() restart) before Wren runs at all.
	// Desktop-editor builds start Wren once here too, purely so the initial scene is loaded and visible in the
	// Game Viewport before Play is first pressed - GameLoop's Update() stays gated on debugSettings.playingGame
	// on both platforms either way, so nothing actually simulates until Play/Refresh is clicked.
#if !defined(EDITOR)
	wrenStateManager.Start(context);
#endif
}

void Struktur::ExitGame(GameContext& context)
{
#ifdef EDITOR
	DEBUG_INFO("[Clean Up] Editor");
	Debug::Editor& editor = context.GetEditor();
	editor.Shutdown(context);
#endif

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

	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();

	DEBUG_INFO("[Clean Up] Dialogue Registry");
	Dialogue::DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
	dialogueRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

	DEBUG_INFO("[Clean Up] DialogueManager");
	Dialogue::DialogueManager& dialogueManager = context.GetDialogueManager();
	dialogueManager.Clear();

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	resourceManager.Clear();

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

	Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFont(context, SPLASHSCREENFONT, 120);
	// Pinned once (see ResourcePool<T>::Pin) instead of holding a persistent ResourcePtr somewhere for the whole
	// splash duration - without this, GetResource()'s cache would evict/reload the font every single frame, since
	// `font` above is the only reference to it and it goes out of scope at the end of this function. Pinning lets
	// this stay a plain per-frame re-fetch with nothing to own long-term.
	static bool fontPinned = false;
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

	context.GetGraphicsDevice().BeginFrame();
	// In EDITOR builds, GameViewportWindow::Initialise already redirected World/Debug/UI views into the
	// editor's offscreen game-viewport framebuffer before this loop ever runs - drawing there would be
	// invisible since nothing composites that texture onto the real window until editor.Update() runs, which
	// this splash-only loop never calls. Bypass the redirect so this frame's UI draws land on the real
	// backbuffer instead - both calls are safe no-ops on builds that never redirected in the first place (no
	// editor, or web).
	context.GetGraphicsDevice().ResetWorldRenderTarget();
	context.GetUIRenderer().SetupView(context);
	context.GetUIRenderer().DrawText(font->font, splashScreenName,
	                                 {(float)((width - fontWidth) / 2), (float)((height - fontSize) / 2)},
	                                 (float)fontSize, Util::Color{255, 255, 255, (unsigned char)textAlpha});
	context.GetGraphicsDevice().EndFrame();
	// bgfx view state (framebuffer/clear/rect) isn't snapshotted at submit time - it's whatever was last set
	// before bgfx::frame() runs. Restoring the redirect must happen AFTER EndFrame() (this frame's draws are
	// already handed off by then) so it takes effect starting with GameLoop's first frame, not this one -
	// restoring it before EndFrame() would silently redirect this frame's own draws right back offscreen.
	if (gameData.gameState == Core::GameState::GAME)
	{
		context.GetGraphicsDevice().RestoreWorldRenderTarget();
		// Splash is done with the font - unpin now rather than leaving it resident for the rest of the process
		// (the old Game()-level ResourcePtr this replaced did the latter).
		font.Unpin();
		fontPinned = false;
	}
}

void Struktur::GameLoop(GameContext& context)
{
	PROFILE_BEGIN_SCOPE(gameLoop, "GAME LOOP");
	System::SystemManager& systemManager         = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	{
		PROFILE_SCOPE("UPDATE PROCESSING");
#ifdef EDITOR
		auto& debugSettings = context.GetEditor().GetSettings().debugRender;
		if (debugSettings.playingGame && !debugSettings.pausedGame)
		{
#endif
			systemManager.Update(context);
#ifdef EDITOR
		}
#endif
		// flush queues
		gameObjectManager.UpdateGameObjectsActiveStateQueue(context);
		gameObjectManager.DeleteGameObjectsInSafeToDeleteQueue(context);
	}

	context.GetGraphicsDevice().BeginFrame();
#ifdef EDITOR
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	Debug::Editor& editor = context.GetEditor();
#endif
	{
		PROFILE_SCOPE("RENDER PROCESSING");
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
	context.GetWindow().PollEvents();
	context.GetInput().Update();
	gameData.applicationWidth  = context.GetWindow().GetWidth();
	gameData.applicationHeight = context.GetWindow().GetHeight();
	if (context.GetWindow().WasResized())
	{
		context.GetGraphicsDevice().Resize(gameData.applicationWidth, gameData.applicationHeight);
	}
	timeSystem.Update();

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

	PROFILE_END_FRAME();
}

void Struktur::Game()
{
	GameContext context;

	// Load resources
	InitialiseGame(context);

	Core::GameData& gameData = context.GetGameData();

#ifdef PLATFORM_WEB
	// Web platform - use emscripten main loop
	emscripten_set_main_loop_arg(UpdateLoop, &context, 0, 1);
#else
	// Desktop platform - standard game loop (frame pacing comes from GraphicsDevice's BGFX_RESET_VSYNC)
	while (gameData.gameState != Core::GameState::QUIT)
	{
		UpdateLoop(&context);
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
	DEBUG_INFO("[Clean Up] FlagManager");
	Flag::FlagManager& flagManager = context.GetFlagManager();
	flagManager.Clear();

	DEBUG_INFO("[Clean Up] Input");
	Input::Input& input = context.GetInput();
	input.Clear();

	DEBUG_INFO("[Clean Up] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Shutdown(context);

	DEBUG_INFO("[Clean Up] UI Manager");
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Clear(context);

	DEBUG_INFO("[Clean Up] Registry");
	entt::registry& registry = context.GetRegistry();
	registry.clear();

	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();

	DEBUG_INFO("[Clean Up] Dialogue Registry");
	Dialogue::DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
	dialogueRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

	DEBUG_INFO("[Clean Up] Dialogue Manager");
	Dialogue::DialogueManager& dialogueManager = context.GetDialogueManager();
	dialogueManager.Clear();

	#ifdef EDITOR
	DEBUG_INFO("[Clean Up] Debug System");
	System::DebugSystem& debugSystem = context.GetSystemManager().GetSystem<System::DebugSystem>();
	debugSystem.ClearCachedResources();
	#endif

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();

	// Everything above (UI Manager, Wren VM shutdown's finalizers, DebugSystem's cached font, etc.) should have
	// already released every ResourcePtr it was holding by this point - if a pool still shows resources loaded
	// here, something is leaking a reference instead of letting it go out of scope/be explicitly unloaded. Assert
	// now, while it's still attributable to a specific pool, rather than letting Clear() below silently force-free
	// it out from under whatever's still holding it.
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

	resourceManager.Clear();
}

void Struktur::StartGameSystems(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();

	DEBUG_INFO("[Start] Wren State Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Initialise(context);

	DEBUG_INFO("[Start] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Initialise(context);

	DEBUG_INFO("[Start] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.LoadAllScriptComponents(context);

	DEBUG_INFO("[Start] Physics World");
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	glm::vec2 gravity(0.0f, 0.0f);
	physicsWorld.Initialise(gravity, gameData.velocityIterations, gameData.positionIterations, gameData.pixelsPerMeter);
}
#endif
