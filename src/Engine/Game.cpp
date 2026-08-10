#include "Game.h"

#include <memory>
#include <string>
#include <variant>

#include "raylib.h"
#include "raymath.h"
#ifdef PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
#endif

#include "Debug/Assertions.h"
#include "Debug/Profiling/Profiler.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Player.h"
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
#include "Engine/Game/Level.h"
#include "Engine/GameContext.h"
#include "Engine/Input/Input.h"
#include "Engine/Scripting/WrenScriptEngine.h"

#ifdef DEBUG
	#include "Engine/Scripting/WrenCodeGenerator.h"
	#if defined(PLATFORM_WEB)
		#include "rlImGui.h"
	#endif
#endif

#if !defined(PLATFORM_WEB)
	#include "Engine/Platform/Window.h"
	#include "Engine/Renderer/GraphicsDevice.h"
	#ifdef EDITOR
		#include <imgui.h>
		#include <imgui_impl_sdl3.h>
	#endif
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

	FileSystem::Init(::GetWorkingDirectory());
#if defined(PLATFORM_WEB)
	// Emscripten preloads assets into its virtual FS
	// Mount the preloaded assets directory directly
	FileSystem::Mount("/assets");
#elif defined(EDITOR)
	// Project folder selection at startup
	std::string projectPath = FileSystem::OpenFolderDialog("Select Project Directory", ::GetWorkingDirectory());

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
	#if defined(PLATFORM_WEB)
	::InitWindow(windowWidth, windowHeight, "Struktur");
	::SetWindowState(FLAG_WINDOW_RESIZABLE);

	::rlImGuiSetup(true);
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	Debug::Editor& editor = context.GetEditor();
	editor.Initialise(context);
	#else
	context.InitialiseGraphics(windowWidth, windowHeight, "Struktur", true);
	context.InitialiseUIRenderer();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// "ForOther" variant: handles input/platform (mouse, keyboard, clipboard, cursors) but expects a separate
	// renderer backend, which is our own hand-rolled bgfx one (Engine/Renderer/ImGuiRenderer) rather than an
	// official imgui_impl_* target.
	ImGui_ImplSDL3_InitForOther(context.GetWindow().GetSDLWindow());
	// Window::PollEvents() drains the SDL event queue itself and only exposes close/resize - this callback is
	// how ImGui sees the raw events it needs (text input, wheel, focus) that polled state can't provide.
	context.GetWindow().SetEventCallback([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });
	context.InitialiseImGuiRenderer();

	Debug::Editor& editor = context.GetEditor();
	editor.Initialise(context);
	#endif
#endif

	wrenScriptEngine.Initialise(context);
	wrenStateManager.Initialise(context);
	wrenScriptComponentRegistry.LoadAllScriptComponents(context);

#ifndef EDITOR
	// In release mode, window matches game size
	#if defined(PLATFORM_WEB)
	::InitWindow(gameData.gameWidth, gameData.gameHeight, gameData.projectName);
	if (gameData.isFullScreen != ::IsWindowFullscreen())
	{
		::ToggleFullscreen();
	}
	#else
	context.InitialiseGraphics(gameData.gameWidth, gameData.gameHeight, gameData.projectName, false);
	context.InitialiseUIRenderer();
	if (gameData.isFullScreen)
	{
		context.GetWindow().SetFullscreen(true);
	}
	#endif
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
#if defined(PLATFORM_WEB)
	::SetExitKey(KEY_NULL);
#endif
	// Audio setup now happens in GameContext's constructor via SDL3_mixer, so it's already available here.

#ifdef DEBUG
	Wren::CodeGenerator::GenerateBindingFiles(wrenScriptEngine.GetRegistry(),
	                                          std::string(::GetWorkingDirectory()) + "/../src/WrenBindings/Bindings");
#endif

	gameObjectManager.CreateObjectCallBack(context);

	glm::vec2 gravity(0.0f, 0.0f);
	physicsWorld.Initialise(gravity, gameData.velocityIterations, gameData.positionIterations, gameData.pixelsPerMeter);

	// The order here also defines the order they are updated - TODO need a better way to determine render priority and
	// also need a way to have helper systems with out an empty update
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
	// WrenStateRenderSystem/DebugSystem still render via direct raylib calls (Wren-script-driven draws and
	// debug shapes) with no bgfx path yet - guarded out on desktop until later steps land; unaffected on web.
	// UIRenderSystem now has a bgfx path on both platforms (see Engine/Renderer/UIRenderer).
#if defined(PLATFORM_WEB)
	systemManager.AddRenderSystem<System::WrenStateRenderSystem>();
	#ifdef DEBUG
	systemManager.AddRenderSystem<System::DebugSystem>();
	#endif
#endif
	systemManager.AddRenderSystem<System::UIRenderSystem>();

	DEBUG_INFO("Game Data Loaded");

	// Release builds always start Wren immediately. Web-editor builds wait for the toolbar's Play button
	// (StartDebugGame - a full ClearGameSystems + StartGameSystems + Start() restart) before Wren runs at all.
	// Desktop-editor builds start Wren once here too, purely so the initial scene is loaded and visible in the
	// Game Viewport before Play is first pressed - GameLoop's Update() stays gated on debugSettings.playingGame
	// on both platforms either way, so nothing actually simulates until Play/Refresh is clicked.
#if !defined(EDITOR) || !defined(PLATFORM_WEB)
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
		textAlpha *= Lerp(0.f, 1.f, t);
	}
	// Fade out
	else if (currentTime > startTime + fadeInTime + holdTime &&
	         currentTime < startTime + fadeInTime + holdTime + fadeOutTime)
	{
		float t = (currentTime - startTime - fadeInTime - holdTime) / fadeOutTime;
		textAlpha *= Lerp(1.f, 0.f, t);
	}

	std::string splashScreenName = SPLASHSCREENTEXT;
	int fontSize                 = 120;

#if defined(PLATFORM_WEB)
	Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFont(context, SPLASHSCREENFONT, 120);
	int fontWidth = ::MeasureTextEx(font->font, splashScreenName.c_str(), fontSize, 1.0f).x;
	int width     = gameData.applicationWidth;
	int height    = gameData.applicationHeight;

	::BeginDrawing();
	::ClearBackground(BLACK);
	::DrawTextEx(font->font, splashScreenName.c_str(), {(width - fontWidth) / 2.f, (height - fontSize) / 2.f}, fontSize,
	             5.0f, Color{255, 255, 255, (unsigned char)textAlpha});
	::EndDrawing();
#else
	// Text rendering isn't ported yet (FontResource stays stubbed this step, see the plan) - not calling
	// resourceManager.GetFont() here either, since raylib's font loading needs an initialised rlgl context to
	// create the glyph atlas GPU texture, which no longer exists on this path. Timing/fade/state-transition
	// logic above still runs, just nothing loads or draws.
	(void)resourceManager;
#endif
}

void Struktur::GameLoop(GameContext& context)
{
	PROFILE_BEGIN_SCOPE(gameLoop, "GAME LOOP");
	System::SystemManager& systemManager         = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	{
		PROFILE_SCOPE("UPDATE PROCESSING");
#ifdef EDITOR
		// The Toolbar's Play/Pause/Stop buttons are reachable on both platforms now (desktop got a real ImGui-bgfx
		// backend) - Stop clears input bindings/Wren state via ClearGameSystems, so systems that assume bindings
		// exist (UISystem's GetInputAxis2, etc.) must not run again until Play/Refresh reloads them.
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

#if defined(PLATFORM_WEB)
	::BeginDrawing();
	#ifdef EDITOR
	::ClearBackground(DARKGRAY);
	Debug::Editor& editor = context.GetEditor();
	editor.BeginUpdateLoop(context);
	#endif
	{
		PROFILE_SCOPE("RENDER PROCESSING");
		::ClearBackground(BLACK);
		systemManager.Render(context);
	}
	PROFILE_END_SCOPE(gameLoop);  // Must close the scope before the editor is updated
	#ifdef EDITOR
	editor.EndUpdateLoop(context);
	editor.Update(context);
	#endif
	::EndDrawing();
#else
	context.GetGraphicsDevice().BeginFrame();
	#ifdef EDITOR
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	Debug::Editor& editor = context.GetEditor();
	editor.BeginUpdateLoop(context);
	#endif
	{
		PROFILE_SCOPE("RENDER PROCESSING");
		systemManager.Render(context);
	}
	PROFILE_END_SCOPE(gameLoop);
	#ifdef EDITOR
	editor.EndUpdateLoop(context);
	editor.Update(context);
	#endif
	context.GetGraphicsDevice().EndFrame();
#endif
}

void Struktur::UpdateLoop(void* userData)
{
	GameContext& context = *static_cast<GameContext*>(userData);
	PROFILE_BEGIN_FRAME();

	Core::GameData& gameData     = context.GetGameData();
	Core::TimeSystem& timeSystem = context.GetTimeSystem();
#if defined(PLATFORM_WEB)
	gameData.applicationWidth  = ::GetScreenWidth();
	gameData.applicationHeight = ::GetScreenHeight();
#else
	context.GetWindow().PollEvents();
	context.GetInput().Update();
	gameData.applicationWidth  = context.GetWindow().GetWidth();
	gameData.applicationHeight = context.GetWindow().GetHeight();
	if (context.GetWindow().WasResized())
	{
		context.GetGraphicsDevice().Resize(gameData.applicationWidth, gameData.applicationHeight);
	}
#endif
	timeSystem.Update();

#if !defined(PLATFORM_WEB)
	if (context.GetWindow().ShouldClose())
	{
		gameData.gameState = Core::GameState::QUIT;
	}
#endif

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
			// On web, clean up and stop the loop
			ExitGame(context);
			::CloseWindow();
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

	// create local scope to manage lifetime of splash screen font - TODO create a spash screen state and add it to the
	// context.
#if defined(PLATFORM_WEB)
	Resource::ResourceManager& resourceManager         = context.GetResourceManager();
	Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFont(context, SPLASHSCREENFONT, 120);
#endif

#ifdef PLATFORM_WEB
	// Web platform - use emscripten main loop
	emscripten_set_main_loop_arg(UpdateLoop, &context, 0, 1);
#else
	// Desktop platform - standard game loop (frame pacing comes from GraphicsDevice's BGFX_RESET_VSYNC, not a
	// raylib-style SetTargetFPS - there's no equivalent knob wired up yet without vsync being the only control).
	while (gameData.gameState != Core::GameState::QUIT)
	{
		UpdateLoop(&context);
	}
#endif

	// Cleanup
	ExitGame(context);
#if defined(PLATFORM_WEB)
	#ifdef EDITOR
	::rlImGuiEnd();
	#endif
	::CloseWindow();
#else
	#ifdef EDITOR
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	#endif
	// GameContext's Window/GraphicsDevice/ImGuiRenderer unique_ptrs tear themselves down when context is destroyed.
#endif
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

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
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
