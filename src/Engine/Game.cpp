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

constexpr static const unsigned int FPS = 60;
constexpr static const float TIME_STEP = 1.0f / FPS;
constexpr static const int VELOCITY_ITERATIONS = 16;
constexpr static const int POSITION_ITERATIONS = 10;
constexpr static const float PIXELS_PER_METER = 64.f;
constexpr static const char* INPUT_BINDINGS_PATH = "assets/Settings/InputBindings/InputBindings.xml";

void Struktur::InitialiseGame(GameContext& context)
{
    Core::Input& input = context.GetInput();
    System::SystemManager& systemManager = context.GetSystemManager();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
    GameResource::StateManager& stateManager = context.GetStateManager();
    Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
    Wren::WrenScriptEngine& wrenScriptEngine = context.GetWrenScriptEngine();
    wrenScriptEngine.Initialize(context);

#ifdef DEBUG
    Wren::CodeGenerator::GenerateBindingFiles("Assets/Scripts/Bindings");
#endif

    gameObjectManager.CreateDeleteObjectCallBack(context);

    input.LoadInputBindings(INPUT_BINDINGS_PATH);
    glm::vec2 gravity(0.0f, 0.0f);
    physicsWorld.Initialise(gravity, VELOCITY_ITERATIONS, POSITION_ITERATIONS, PIXELS_PER_METER);

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

    std::unique_ptr<GamePlay::GameWorldState> gameWorldState = std::make_unique<GamePlay::GameWorldState>();
    stateManager.ChangeState(context, std::move(gameWorldState));
}

void Struktur::ExitGame(GameContext &context)
{
#ifdef EDITOR
    DEBUG_INFO("[Clean Up] Editor");
    Debug::Editor& editor = context.GetEditor();
    editor.Shutdown(context);
#endif

    DEBUG_INFO("[Clean Up] State Manager");
    GameResource::StateManager& stateManager = context.GetStateManager();
    stateManager.ReleaseState(context);

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

void Struktur::GameLoop(GameContext &context)
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

    switch(gameData.gameState)
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
    // Game settings
    const int gameWidth = 1280;
    const int gameHeight = 720;
    
    #ifdef EDITOR
    // In debug mode, create a larger window to fit ImGui panels
    const int windowWidth = 1280;
    const int windowHeight = 720;
    ::InitWindow(windowWidth, windowHeight, "Struktur");
    ::SetWindowState(FLAG_WINDOW_RESIZABLE);
    ::rlImGuiSetup(true);
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    #else
    // In release mode, window matches game size
    ::InitWindow(gameWidth, gameHeight, "Memory Palace");
    #endif
    
    ::SetExitKey(KEY_NULL);
    
    GameContext context;

    Core::GameData& gameData = context.GetGameData();
    gameData.gameWidth = gameWidth;
    gameData.gameHeight = gameHeight;

    // Load resources
    InitialiseGame(context);

    Resource::ResourceManager& resourceManager = context.GetResourceManager();
    Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120");
    
    gameData.startTime = ::GetTime();
#ifdef PLATFORM_WEB
    // Web platform - use emscripten main loop
    emscripten_set_main_loop_arg(UpdateLoop, &context, 0, 1);
#else
    // Desktop platform - standard game loop
    ::SetTargetFPS(FPS);
    
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
