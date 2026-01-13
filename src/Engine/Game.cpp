#include "Game.h"

#include <string>
#include <memory>
#include <variant>
#include "raylib.h"
#include "raymath.h"
#ifdef PLATFORM_WEB
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "Debug/Assertions.h"
#include "Debug/Profiling/Profiler.h"

#include "Engine/GameContext.h"
#include "Engine/Core/Input.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/ECS/SystemManager.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/Camera.h"

#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/System/HierrarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/SpriteRenderSystem.h"
#include "Engine/ECS/System/DebugSystem.h"
#include "Engine/ECS/System/CameraSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/UIsystem.h"
#include "Engine/ECS/System/UIRenderSystem.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/System/WrenStateSystem.h"
#include "Engine/ECS/System/SoundSystem.h"

#include "Engine/FileLoading/LevelParser.h"
#include "Engine/Scripting/WrenCodeGenerator.h"

#include "Engine/Game/Level.h"

#ifdef DEBUG
#include "rlImGui.h"
#endif

#ifdef EDITOR
#define SPLASHSCREENFONT "assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf"
#define SPLASHSCREENTEXT "STRUKTUR"
#else
#define SPLASHSCREENFONT "assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf"
#define SPLASHSCREENTEXT "Memory Palace"
#endif

void Struktur::InitialiseGame(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	System::SystemManager& systemManager = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();

	// Want to create a window before we start initialising systems
#ifdef EDITOR
	// In debug mode, create a larger window to fit ImGui panels
	const int windowWidth = 1280;
	const int windowHeight = 720;
	gameData.gameWidth = windowWidth;
	gameData.gameHeight = windowHeight;
	::InitWindow(windowWidth, windowHeight, "Struktur");
	::SetWindowState(FLAG_WINDOW_RESIZABLE);

	::rlImGuiSetup(true);
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	Debug::Editor& editor = context.GetEditor();
	editor.Initialise(context);
#endif

	wrenScriptEngine.Initialise(context);
	wrenStateManager.Initialise(context);
	wrenScriptComponentRegistry.LoadAllScriptComponents(context);

#ifndef EDITOR
	// In release mode, window matches game size
	::InitWindow(gameData.gameWidth, gameData.gameHeight, gameData.projectName);
#endif
	::SetExitKey(KEY_NULL);
	::InitAudioDevice();

#ifdef DEBUG
	Wren::CodeGenerator::GenerateBindingFiles("Assets/Scripts/Bindings");
#endif

	gameObjectManager.CreateDeleteObjectCallBack(context);

	glm::vec2 gravity(0.0f, 0.0f);
	physicsWorld.Initialise(gravity, gameData.velocityIterations, gameData.positionIterations, gameData.pixelsPerMeter);

	// The order here also defines the order they are updated - TODO need a better way to determine render priority and also need a way to have helper systems with out an empty update
	systemManager.AddHelperSystem<System::HierarchySystem>();
	systemManager.AddHelperSystem<System::TransformSystem>();
	systemManager.AddHelperSystem<System::ShaderSystem>();
	systemManager.AddUpdateSystem<System::WrenStateSystem>();
	systemManager.AddUpdateSystem<System::WrenScriptSystem>();
	systemManager.AddUpdateSystem<System::CameraSystem>();
	systemManager.AddUpdateSystem<System::PhysicsSystem>();
	systemManager.AddUpdateSystem<System::AnimationSystem>();
	systemManager.AddUpdateSystem<System::UISystem>();
	systemManager.AddUpdateSystem<System::SoundSystem>();
	systemManager.AddRenderSystem<System::SpriteRenderSystem>();
	systemManager.AddRenderSystem<System::WrenStateRenderSystem>();
#ifdef DEBUG
	systemManager.AddRenderSystem<System::DebugSystem>();
#endif
	systemManager.AddRenderSystem<System::UIRenderSystem>();

	DEBUG_INFO("Game Data Loaded");

#ifndef EDITOR
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
	uiManager.Clear();

	DEBUG_INFO("[Clean Up] Registry");
	entt::registry& registry = context.GetRegistry();
	registry.clear();

	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	resourceManager.Clear();
}

void Struktur::SplashScreenLoop(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFont(SPLASHSCREENFONT, 120);
	//fade in time
	const double fadeInTime = 1.5;
	const double holdTime = 1;
	const double fadeOutTime = 1.5;
	const double currentTime = gameData.gameTime;
	const double startTime = gameData.startTime;
	if (currentTime > startTime + fadeInTime + holdTime + fadeOutTime)
	{
		gameData.gameState = Core::GameState::GAME;
#ifdef PLATFORM_WEB
		emscripten_cancel_main_loop();
#endif
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
	else if (currentTime > startTime + fadeInTime + holdTime && currentTime < startTime + fadeInTime + holdTime + fadeOutTime)
	{
		float t = (currentTime - startTime - fadeInTime - holdTime) / fadeOutTime;
		textAlpha *= Lerp(1.f, 0.f, t);
	}

	std::string splashScreenName = SPLASHSCREENTEXT;
	int fontSize = 120;
	int fontWidth = ::MeasureTextEx(font->font, splashScreenName.c_str(), fontSize, 1.0f).x;
	int width = gameData.applicationWidth;
	int height = gameData.applicationHeight;

	::BeginDrawing();
	::ClearBackground(BLACK);
	::DrawTextEx(font->font, splashScreenName.c_str(), { (width - fontWidth) / 2.f, (height - fontSize) / 2.f }, fontSize, 5.0f, Color{ 255,255,255,(unsigned char)textAlpha });
	::EndDrawing();
}

void Struktur::GameLoop(GameContext& context)
{
	System::SystemManager& systemManager = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	{
		PROFILE_SCOPE("Update Processing");
#ifdef EDITOR
		auto& debugSettings = context.GetEditor().GetSettings().debugRender;
		if (debugSettings.playingGame && !debugSettings.pausedGame)
		{
#endif
		systemManager.Update(context);
#ifdef EDITOR
		}
#endif
		gameObjectManager.DeleteGameObjectsInSafeToDeleteQueue(context);
	}

	{
		PROFILE_SCOPE("Render Processing");
		::BeginDrawing();
#ifdef EDITOR
		::ClearBackground(DARKGRAY);
		Debug::Editor& editor = context.GetEditor();
		editor.BeginUpdateLoop(context);
#endif
		::ClearBackground(BLACK);
		systemManager.Render(context);
#ifdef EDITOR
		editor.EndUpdateLoop(context);
		editor.Update(context);
#endif
		::EndDrawing();
	}
}

void Struktur::SplashScreenUpdateLoop(void* userData)
{
	auto* context = static_cast<GameContext*>(userData);
	// Set the game data
	Core::GameData& gameData = context->GetGameData();
	gameData.deltaTime = ::GetFrameTime();
	gameData.gameTime = ::GetTime();
	gameData.applicationWidth = ::GetScreenWidth();
	gameData.applicationHeight = ::GetScreenHeight();

#ifndef PLATFORM_WEB
	if (::WindowShouldClose())
	{
		gameData.gameState = Core::GameState::QUIT;
	}
#endif

	switch (gameData.gameState)
	{
	case Core::GameState::SPLASH_SCREEN:
		SplashScreenLoop(*context);
		break;
#ifdef PLATFORM_WEB
	default:
		emscripten_cancel_main_loop();
#endif
	}
}

void Struktur::MainUpdateLoop(void* userData)
{
	PROFILE_BEGIN_FRAME();
	GameContext* context = static_cast<GameContext*>(userData);
	// Set the game data
	Core::GameData& gameData = context->GetGameData();
	gameData.deltaTime = ::GetFrameTime();
	gameData.gameTime = ::GetTime();
	gameData.applicationWidth = ::GetScreenWidth();
	gameData.applicationHeight = ::GetScreenHeight();

#ifndef PLATFORM_WEB
	if (::WindowShouldClose())
	{
		gameData.gameState = Core::GameState::QUIT;
	}
#endif

	switch (gameData.gameState)
	{
	case Core::GameState::GAME:
		GameLoop(*context);
		break;
	default:
#ifdef PLATFORM_WEB
		emscripten_cancel_main_loop();
#endif
		gameData.gameState = Core::GameState::QUIT;
	}
	PROFILE_END_FRAME();
}

void Struktur::Game()
{
	GameContext context;

	// Load resources
	InitialiseGame(context);

	Core::GameData& gameData = context.GetGameData();
	gameData.startTime = ::GetTime();
	{
		// create local scope to manage lifetime of splash screen font - TODO create a spash screen state and add it to the context.
		Resource::ResourceManager& resourceManager = context.GetResourceManager();
		Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFont(SPLASHSCREENFONT, 120);

#ifdef PLATFORM_WEB
		// Web platform - use emscripten main loop
		emscripten_set_main_loop_arg(SplashScreenUpdateLoop, &context, 0, 1);
#else
		// Desktop platform - standard game loop
		::SetTargetFPS(gameData.fps);

		while (gameData.gameState == Core::GameState::SPLASH_SCREEN) {
			SplashScreenUpdateLoop(&context);
		}
#endif
	}

#ifdef PLATFORM_WEB
	// Web platform - use emscripten main loop
	emscripten_set_main_loop_arg(MainUpdateLoop, &context, 0, 1);
#else
	// Desktop platform - standard game loop
	::SetTargetFPS(gameData.fps);

	while (gameData.gameState != Core::GameState::QUIT) {
		MainUpdateLoop(&context);
	}
#endif

	// Cleanup
	ExitGame(context);
#ifdef EDITOR
	::rlImGuiEnd();
#endif
	::CloseWindow();
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
	DEBUG_INFO("[Clean Up] Input");
	Core::Input& input = context.GetInput();
	input.Clear();

	DEBUG_INFO("[Clean Up] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Shutdown(context);

	DEBUG_INFO("[Clean Up] UI Manager");
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Clear();

	DEBUG_INFO("[Clean Up] Registry");
	entt::registry& registry = context.GetRegistry();
	registry.clear();

	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();

	DEBUG_INFO("[Clean Up] Wren Script Component Registry");
	Wren::WrenScriptComponentRegistry& wrenScriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	wrenScriptComponentRegistry.Clear(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

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
