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
#include "Engine/ECS/SystemManager.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"

#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/System/HierrarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/PlayerSystem.h"
#include "Engine/ECS/System/SpriteRenderSystem.h"
#include "Engine/ECS/System/DebugSystem.h"

constexpr static const unsigned int FPS = 60;
constexpr static const float TIME_STEP = 1.0f / FPS;
constexpr static const int VELOCITY_ITERATIONS = 6;
constexpr static const int POSITION_ITERATIONS = 4;
constexpr static const char* CIRCLE_TEXTURE = "assets/Circle.png";
constexpr static const char* PLAYERIDLE_TEXTURE = "assets/32x32idle.png";

void Struktur::LoadData(GameContext& context)
{
    Core::Input& input = context.GetInput();
    entt::registry& registry = context.GetRegistry();
    Core::ResourcePool& resourcePool = context.GetResourcePool();
    System::SystemManager& systemManager = context.GetSystemManager();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();

    gameObjectManager.CreateDeleteObjectCallBack(context);

    // The order here also defines the order they are updated
    systemManager.AddUpdateSystem<System::PlayerSystem>();
    systemManager.AddUpdateSystem<System::HierarchySystem>();
    systemManager.AddUpdateSystem<System::TransformSystem>();
    auto& physicsSystem = systemManager.AddUpdateSystem<System::PhysicsSystem>();
    systemManager.AddRenderSystem<System::SpriteRenderSystem>();
    systemManager.AddRenderSystem<System::DebugSystem>();

    input.LoadInputBindings("assets/Settings/InputBindings/InputBindings.xml");
    DEBUG_INFO("Game Data Loaded");

    auto parent = gameObjectManager.CreateGameObject(context);
    registry.emplace<Component::Player>(parent, 10.f);
    registry.emplace<Component::Sprite>(parent, PLAYERIDLE_TEXTURE, WHITE, glm::vec2(16, 16), 4, 1, false, 0);
    auto& parentTransform = registry.get<Component::Transform>(parent);
    parentTransform.position = {500.0f, 300.0f, 0.0f};
    b2BodyDef kinematicBodyDef;
    kinematicBodyDef.type = b2_dynamicBody;
    physicsSystem.CreatePhysicsBody(context, parent, kinematicBodyDef);
    auto& physicsBody = registry.get<Component::PhysicsBody>(parent);
    physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody.syncToPhysics = true;     // Let transform drive physics

    // Create children
    auto child1 = gameObjectManager.CreateGameObject(context, parent);
    registry.emplace<Component::Sprite>(child1, CIRCLE_TEXTURE, GREEN, glm::vec2(8, 8), 2, 2, false, 1);
    auto& child1Transform = registry.get<Component::Transform>(child1);
    child1Transform.position = {50.0f, 10.0f, 0.0f}; // Relative to parent
    
    auto child2 = gameObjectManager.CreateGameObject(context, parent);
    registry.emplace<Component::Sprite>(child2, CIRCLE_TEXTURE, BLUE, glm::vec2(16, 16), 1, 1, false, 0);
    auto& child2Transform = registry.get<Component::Transform>(child2);
    child2Transform.position = {-100.0f, -90.0f, 0.0f};
    
    auto wall = gameObjectManager.CreateGameObject(context);
    registry.emplace<Component::Sprite>(wall, CIRCLE_TEXTURE, RED, glm::vec2(16, 16), 1, 1, false, 0);
    auto& wallTransform = registry.get<Component::Transform>(wall);
    wallTransform.position = {300.0f, 200.0f, 0.0f};
    b2BodyDef kinematicBody2Def;
    kinematicBody2Def.type = b2_staticBody;
    physicsSystem.CreatePhysicsBody(context, wall, kinematicBody2Def);
    auto& physicsBody2 = registry.get<Component::PhysicsBody>(wall);
    physicsBody2.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody2.syncToPhysics = true;     // Let transform drive physics
    
    auto movingBox = gameObjectManager.CreateGameObject(context);
    registry.emplace<Component::Sprite>(movingBox, CIRCLE_TEXTURE, YELLOW, glm::vec2(16, 16), 1, 1, false, 0);
    auto& movingBoxTransform = registry.get<Component::Transform>(movingBox);
    movingBoxTransform.position = {700.0f, 700.0f, 0.0f};
    b2BodyDef kinematicBody3Def;
    kinematicBody3Def.type = b2_dynamicBody;
    physicsSystem.CreatePhysicsBody(context, movingBox, kinematicBody3Def);
    auto& physicsBody3 = registry.get<Component::PhysicsBody>(movingBox);
    physicsBody3.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody3.syncToPhysics = true;     // Let transform drive physics

	DEBUG_INFO("Created Player Entity");

    resourcePool.CreateTexture(CIRCLE_TEXTURE);
    resourcePool.LoadTextureInGPU(CIRCLE_TEXTURE);

    resourcePool.CreateTexture(PLAYERIDLE_TEXTURE);
    resourcePool.LoadTextureInGPU(PLAYERIDLE_TEXTURE);
}

void Struktur::SplashScreenLoop(GameContext& context)
{
    Core::GameData& gameData = context.GetGameData();
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

void Struktur::LoadingLoop(GameContext& context)
{

}

void Struktur::GameLoop(GameContext& context)
{
    Core::GameData& gameData = context.GetGameData();
    entt::registry& registry = context.GetRegistry();
    Core::ResourcePool& resourcePool = context.GetResourcePool();
    System::SystemManager& systemManager = context.GetSystemManager();

#ifndef PLATFORM_WEB
    if (WindowShouldClose()) 
    {
        gameData.gameState = Core::GameState::QUIT;
    }
#endif

    systemManager.Update(context);    
}

void Struktur::UpdateLoop(void* userData) 
{
    GameContext* context = static_cast<GameContext*>(userData);
    // Set the game data
    Core::GameData& gameData = context->GetGameData();
    gameData.deltaTime = ::GetFrameTime();
    gameData.gameTime = ::GetTime();
    gameData.screenWidth = ::GetScreenWidth();
    gameData.screenHeight = ::GetScreenHeight();
    
    switch(gameData.gameState)
    {
    case Core::GameState::SPLASH_SCREEN:
        SplashScreenLoop(*context);
        return;
    case Core::GameState::LOADING:
        LoadingLoop(*context);
        return;
    case Core::GameState::GAME:
        GameLoop(*context);
        return;
    }
}

void Struktur::Game() 
{
    // Initialize window
    const int screenWidth = 1024;
    const int screenHeight = 768;

    GameContext context;
    
    ::InitWindow(screenWidth, screenHeight, "Struktur");
    ::SetExitKey(KEY_NULL);

    // Load resources
    LoadData(context);
    
    Core::GameData& gameData = context.GetGameData();
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
    ::CloseWindow();
}
