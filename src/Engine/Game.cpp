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

#include "Engine/FileLoading/LevelParser.h"
#include "Engine/Scripting/WrenCodeGenerator.h"

#include "Engine/Game/Level.h"

#ifdef DEBUG
#include "rlImGui.h"
#endif

#include "Gameplay/GameplayStates/GameWorldState.h"

void Struktur::InitialiseGame(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	Core::Input& input = context.GetInput();
	System::SystemManager& systemManager = context.GetSystemManager();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	GameResource::StateManager& stateManager = context.GetStateManager();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();

	//TODO probably want to better handle this for this
	::InitWindow(1, 1, "");
	::SetExitKey(KEY_NULL);

	wrenScriptEngine.Initialize(context);

	::rlImGuiSetup(true);
#ifdef EDITOR
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

#ifdef DEBUG
	Wren::CodeGenerator::GenerateBindingFiles("Assets/Scripts/Bindings");
#endif

	gameObjectManager.CreateDeleteObjectCallBack(context);

	input.LoadInputBindings(gameData.inputBindingsPath);
	glm::vec2 gravity(0.0f, 0.0f);
	physicsWorld.Initialise(gravity, gameData.velocityIterations, gameData.positionIterations, gameData.pixelsPerMeter);

#ifdef EDITOR
	Debug::Editor& editor = context.GetEditor();
	editor.Initialise(context);
#endif

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
	systemManager.AddRenderSystem<System::SpriteRenderSystem>();
	systemManager.AddRenderSystem<System::WrenStateRenderSystem>();
#ifdef DEBUG
	systemManager.AddRenderSystem<System::DebugSystem>();
#endif
	systemManager.AddRenderSystem<System::UIRenderSystem>();

	DEBUG_INFO("Game Data Loaded");

	wrenStateManager.Initialize(context);

#ifdef EDITOR
	// In debug mode, create a larger window to fit ImGui panels
	const int windowWidth = 1280;
	const int windowHeight = 720;
	::SetWindowSize(windowWidth, windowHeight);
	::SetWindowTitle("Struktur");
	::SetWindowState(FLAG_WINDOW_RESIZABLE);
#else
	// In release mode, window matches game size
	::SetWindowSize(gameData.gameWidth, gameData.gameHeight);
	::SetWindowTitle(gameData.projectName);
	::SetWindowTitle()
#endif

	//std::unique_ptr<GamePlay::GameWorldState> gameWorldState = std::make_unique<GamePlay::GameWorldState>();
	//stateManager.ChangeState(context, std::move(gameWorldState));
}

void Struktur::ExitGame(GameContext& context)
{
#ifdef EDITOR
	DEBUG_INFO("[Clean Up] Editor");
	Debug::Editor& editor = context.GetEditor();
	editor.Shutdown(context);
#endif

	DEBUG_INFO("[Clean Up] State Manager"); // TODO remove this
	GameResource::StateManager& stateManager = context.GetStateManager();
	stateManager.ReleaseState(context);

	DEBUG_INFO("[Clean Up] Wren State Manager");
	Wren::WrenStateManager& wrenStateManager = context.GetWrenStateManager();
	wrenStateManager.Shutdown(context);

	DEBUG_INFO("[Clean Up] Wren Script Engine");
	Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
	wrenScriptEngine.Shutdown();

	DEBUG_INFO("[Clean Up] UI Manager");
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Clear();

	DEBUG_INFO("[Clean Up] Registry");
	entt::registry& registry = context.GetRegistry();
	registry.clear();

	DEBUG_INFO("[Clean Up] Physics World");
	Physics::PhysicsWorld& world = context.GetPhysicsWorld();
	world.Clear();

	DEBUG_INFO("[Clean Up] Resource Manager");
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	resourceManager.Clear();
}

void Struktur::SplashScreenLoop(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120");
	//fade in time
	const double fadeInTime = 1.5;
	const double holdTime = 1;
	const double fadeOutTime = 1.5;
	const double currentTime = gameData.gameTime;
	const double startTime = gameData.startTime;
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
	else if (currentTime > startTime + fadeInTime + holdTime && currentTime < startTime + fadeInTime + holdTime + fadeOutTime)
	{
		float t = (currentTime - startTime - fadeInTime - holdTime) / fadeOutTime;
		textAlpha *= Lerp(1.f, 0.f, t);
	}

	std::string splashScreenName = "Memory Palace";
	int fontSize = 120;
	int fontWidth = ::MeasureTextEx(font->font, splashScreenName.c_str(), fontSize, 1.0f).x;
	int width = gameData.applicationWidth;
	int height = gameData.applicationHeight;

	::BeginDrawing();
	::ClearBackground(Color{ 0,0,0,255 });
	::DrawTextEx(font->font, splashScreenName.c_str(), { (width - fontWidth) / 2.f, (height - fontSize) / 2.f }, fontSize, 5.0f, Color{ 255,255,255,(unsigned char)textAlpha });
	::EndDrawing();
}

void Struktur::LoadingLoop(GameContext& context)
{

}

void Struktur::GameLoop(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	entt::registry& registry = context.GetRegistry();
	System::SystemManager& systemManager = context.GetSystemManager();

#ifndef PLATFORM_WEB
	if (WindowShouldClose())
	{
		gameData.gameState = Core::GameState::QUIT;
	}
#endif

	systemManager.Update(context);

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

void Struktur::UpdateLoop(void* userData)
{
	GameContext* context = static_cast<GameContext*>(userData);
	// Set the game data
	Core::GameData& gameData = context->GetGameData();
	gameData.deltaTime = ::GetFrameTime();
	gameData.gameTime = ::GetTime();
	gameData.applicationWidth = ::GetScreenWidth();
	gameData.applicationHeight = ::GetScreenHeight();

	switch (gameData.gameState)
	{
	case Core::GameState::SPLASH_SCREEN:
		SplashScreenLoop(*context);
		break;
	case Core::GameState::LOADING:
		LoadingLoop(*context);
		break;
	case Core::GameState::GAME:
		GameLoop(*context);
		break;
	}
}

void Struktur::Game()
{
	GameContext context;

	// Load resources
	InitialiseGame(context);

	Resource::ResourceManager& resourceManager = context.GetResourceManager();
	Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120");

	Core::GameData& gameData = context.GetGameData();
	gameData.startTime = ::GetTime();
#ifdef PLATFORM_WEB
	// Web platform - use emscripten main loop
	emscripten_set_main_loop_arg(UpdateLoop, &context, 0, 1);
#else
	// Desktop platform - standard game loop
	::SetTargetFPS(gameData.fps);

	while (gameData.gameState != Core::GameState::QUIT) {
		UpdateLoop(&context);
	}
#endif

	// Cleanup
	ExitGame(context);
#ifdef EDITOR
	::rlImGuiEnd();
#endif
	::CloseWindow();
}
