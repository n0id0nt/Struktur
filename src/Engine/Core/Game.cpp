#include "Game.h"

#include "raylib.h"
#include "raymath.h"

#include <string>
#include <memory>
#include "Debug/Assertions.h"
#include "Engine/Core/GameContext.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/SystemManager.h"
#include "ECS/Component/Transform.h"
#include "ECS/System/GameObjectManager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#ifdef PLATFORM_WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

constexpr static const unsigned int FPS = 60;
constexpr static const float TIME_STEP = 1.0f / FPS;
constexpr static const int VELOCITY_ITERATIONS = 6;
constexpr static const int POSITION_ITERATIONS = 4;
constexpr static const char* PLAYER_TEXTURE = "assets/Circle.png";

// Create some example components
struct Player {
	float speed;
	Color color;
};

struct WorldTransformData {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    bool valid = false;
};

void Struktur::Core::LoadData(GameContext& context)
{
    Input& input = context.GetInput();
    entt::registry& registry = context.GetRegistry();
    ResourcePool& resourcePool = context.GetResourcePool();
    SystemManager& systemManager = context.GetSystemManager();

    System::GameObjectManager& gameObjectManager = systemManager.AddUpdateSystem<System::GameObjectManager>(context);

    input.LoadInputBindings("assets/Settings/InputBindings/InputBindings.xml");
    DEBUG_INFO("Game Data Loaded");

    auto parent = gameObjectManager.CreateGameObject(context);
    registry.emplace<Player>(parent, 300.f, RED);
    auto& parentTransform = registry.get<Component::Transform>(parent);
    parentTransform.position = {500.0f, 300.0f, 0.0f};

    // Create children
    auto child1 = gameObjectManager.CreateGameObject(context, parent);
    auto& child1Transform = registry.get<Component::Transform>(child1);
    child1Transform.position = {50.0f, 10.0f, 0.0f}; // Relative to parent

    auto child2 = gameObjectManager.CreateGameObject(context, parent);
    auto& child2Transform = registry.get<Component::Transform>(child2);
    child2Transform.position = {-100.0f, -90.0f, 0.0f};

	DEBUG_INFO("Created Player Entity");

    resourcePool.CreateTexture(PLAYER_TEXTURE);
    resourcePool.LoadTextureInGPU(PLAYER_TEXTURE);
}

void Struktur::Core::SplashScreenLoop(GameContext& context)
{
    GameData& gameData = context.GetGameData();
    //fade in time
    const double fadeInTime = 1.5;
    const double holdTime = 1;
    const double fadeOutTime = 1.5;
    const double currentTime = gameData.gameTime;
    const double startTime = gameData.startTime;
    if (currentTime > startTime + fadeInTime + holdTime + fadeOutTime)
    {
        gameData.gameState = GameState::GAME;
        ::TraceLog(LOG_INFO, "Start Game Loop");
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

    std::string splashScreenName = "Struktur";
    int fontSize = 60;
    int fontWidth = ::MeasureText(splashScreenName.c_str(), fontSize);
    int width = gameData.screenWidth;
    int height = gameData.screenHeight;

    ::BeginDrawing();
    ::ClearBackground(Color{ 0,0,0,255 });
    ::DrawText(splashScreenName.c_str(), (width - fontWidth) / 2.f, (height - fontSize) / 2.f, fontSize, Color{ 255,255,255,(unsigned char)textAlpha });
    ::EndDrawing();
}

void Struktur::Core::LoadingLoop(GameContext& context)
{

}

void Struktur::Core::GameLoop(GameContext& context)
{
    GameData& gameData = context.GetGameData();
    entt::registry& registry = context.GetRegistry();
    ResourcePool& resourcePool = context.GetResourcePool();
    SystemManager& systemManager = context.GetSystemManager();

#ifndef PLATFORM_WEB
    if (WindowShouldClose()) 
    {
        gameData.gameState = GameState::QUIT;
    }
#endif
    // player movement system

    glm::vec2 inputDir = context.GetInput().GetInputAxis2("Move");
    auto view = registry.view<Component::Transform, Player>();
    for (auto [entity, transform, player] : view.each())
    {
        transform.position.x += inputDir.x * gameData.dt * player.speed;
        transform.position.y += inputDir.y * gameData.dt * player.speed;
    }

    systemManager.Update(context);
    
    ::BeginDrawing();
    ::ClearBackground(BLACK);
    
    // player render system
    auto view2 = registry.view<Component::WorldTransform>();
    for (auto [entity, worldTransform] : view2.each())
    {
        WorldTransformData data;
        
        glm::vec3 skew;
        glm::vec4 perspective;
        
        glm::decompose(worldTransform.matrix, 
                                   data.scale, 
                                   data.rotation, 
                                   data.position, 
                                   skew, 
                                   perspective);

        ::DrawTexture(resourcePool.RetrieveTexture(PLAYER_TEXTURE), data.position.x, data.position.y, WHITE);
    }
    ::DrawRectangle(300, 300, 30, 30, WHITE);
    ::DrawFPS(20, 20);

    //Struktur::System::Render::Update(gameData.registry, gameData.resourcePool, gameData.camera);
    //Struktur::System::UI::Update(gameData.registry, gameData.resourcePool, gameData.dialogueText, systemTime, gameData.gameState, gameData.previousGameState, gameData.shouldQuit);
    //debug render(lines and stuff)
    //render UI
    //render debug UI
    //Render IMGUI
    ::EndDrawing();
}

void Struktur::Core::UpdateLoop(void* userData) 
{
    GameContext* context = static_cast<GameContext*>(userData);
    // Set the game data
    GameData& gameData = context->GetGameData();
    gameData.dt = ::GetFrameTime();
    gameData.gameTime = ::GetTime();
    gameData.screenWidth = ::GetScreenWidth();
    gameData.screenHeight = ::GetScreenHeight();
    
    switch(gameData.gameState)
    {
    case GameState::SPLASH_SCREEN:
        SplashScreenLoop(*context);
        return;
    case GameState::LOADING:
        LoadingLoop(*context);
        return;
    case GameState::GAME:
        GameLoop(*context);
        return;
    }
}

void Struktur::Core::Game() 
{
    // Initialize window
    const int screenWidth = 1024;
    const int screenHeight = 768;

    GameContext context;
    
    ::InitWindow(screenWidth, screenHeight, "Struktur");
    ::SetExitKey(KEY_NULL);

    // Load resources
    LoadData(context);
    
    GameData& gameData = context.GetGameData();
    gameData.startTime = ::GetTime();
#ifdef PLATFORM_WEB
    // Web platform - use emscripten main loop
    emscripten_set_main_loop_arg(UpdateLoop, &context, 0, 1);
#else
    // Desktop platform - standard game loop
    ::SetTargetFPS(FPS);
    
    while (gameData.gameState != GameState::QUIT) {
        UpdateLoop(&context);
    }
#endif
    
    // Cleanup
    ::CloseWindow();
}