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
#include "Engine/ECS/Component/Camera.h"

#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/System/HierrarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/PlayerSystem.h"
#include "Engine/ECS/System/SpriteRenderSystem.h"
#include "Engine/ECS/System/DebugSystem.h"
#include "Engine/ECS/System/CameraSystem.h"

#include "Engine/Game/Level.h"

#include "Engine/FileLoading/LevelParser.h"

constexpr static const unsigned int FPS = 60;
constexpr static const float TIME_STEP = 1.0f / FPS;
constexpr static const int VELOCITY_ITERATIONS = 6;
constexpr static const int POSITION_ITERATIONS = 4;
constexpr static const char* TILE_TEXTURE = "assets/Tiles/cavesofgallet_tiles.png";
constexpr static const char* PLAYER_TEXTURE = "assets/Tiles/PlayerGrowthSprites.png";

void Struktur::LoadData(GameContext& context)
{
    Core::Input& input = context.GetInput();
    entt::registry& registry = context.GetRegistry();
    Core::ResourcePool& resourcePool = context.GetResourcePool();
    System::SystemManager& systemManager = context.GetSystemManager();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();

    // The order here also defines the order they are updated
    systemManager.AddUpdateSystem<System::PlayerSystem>();
    systemManager.AddUpdateSystem<System::HierarchySystem>();
    auto& transformSystem = systemManager.AddUpdateSystem<System::TransformSystem>();
    systemManager.AddUpdateSystem<System::CameraSystem>();
    auto& physicsSystem = systemManager.AddUpdateSystem<System::PhysicsSystem>();
    systemManager.AddRenderSystem<System::SpriteRenderSystem>();
    systemManager.AddRenderSystem<System::DebugSystem>();

    FileLoading::LevelParser::World world = FileLoading::LevelParser::LoadWorldMap(context, "assets/", "Levels/ExampleLDKTLevel.ldtk"); // need to sore the world somewhere to be refered to later.
    
    FileLoading::LevelParser::Level& firstLevel = world.levels[0]; // should probably actually store the first level somewhere
    GameResource::Level::LoadLevelEntities(context, firstLevel);

    gameObjectManager.CreateDeleteObjectCallBack(context);


    input.LoadInputBindings("assets/Settings/InputBindings/InputBindings.xml");
    DEBUG_INFO("Game Data Loaded");

    auto parent = gameObjectManager.CreateGameObject(context);
    registry.emplace<Component::Player>(parent, 10.f);
    registry.emplace<Component::Sprite>(parent, PLAYER_TEXTURE, WHITE, glm::vec2(16, 16), 12, 5, false, 12);
    auto& parentCamera = registry.emplace<Component::Camera>(parent);
    parentCamera.zoom = 2.f;
    parentCamera.forcePosition = true;
    parentCamera.damping = glm::vec2(0.8f,0.8f);
    transformSystem.SetLocalTransform(context, parent, glm::vec3(500.0f, 300.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    b2BodyDef kinematicBodyDef;
    kinematicBodyDef.type = b2_dynamicBody;
	b2PolygonShape playerShape;
    playerShape.SetAsBox(1 / 2.0f, 1 / 2.0f);
    physicsSystem.CreatePhysicsBody(context, parent, kinematicBodyDef, playerShape);
    auto& physicsBody = registry.get<Component::PhysicsBody>(parent);
    physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody.syncToPhysics = true;     // Let transform drive physics

    // Create children
    auto child1 = gameObjectManager.CreateGameObject(context, parent);
    registry.emplace<Component::Sprite>(child1, TILE_TEXTURE, GREEN, glm::vec2(8, 8), 9, 12, false, 14);
    transformSystem.SetLocalTransform(context, child1, glm::vec3(50.0f, 10.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    
    auto child2 = gameObjectManager.CreateGameObject(context, parent);
    registry.emplace<Component::Sprite>(child2, TILE_TEXTURE, BLUE, glm::vec2(16, 16), 9, 12, false, 3);
    transformSystem.SetLocalTransform(context, child2, glm::vec3(-100.0f, -90.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    
    auto wall = gameObjectManager.CreateGameObject(context);
    registry.emplace<Component::Sprite>(wall, TILE_TEXTURE, RED, glm::vec2(16, 16), 9, 12, false, 5);
    transformSystem.SetLocalTransform(context, wall, glm::vec3(350.0f, 250.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    b2BodyDef kinematicBody2Def;
    kinematicBody2Def.type = b2_staticBody;
	b2PolygonShape wallShape;
    wallShape.SetAsBox(1 / 2.0f, 1 / 2.0f);
    physicsSystem.CreatePhysicsBody(context, wall, kinematicBody2Def, wallShape);
    auto& physicsBody2 = registry.get<Component::PhysicsBody>(wall);
    physicsBody2.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody2.syncToPhysics = true;     // Let transform drive physics
    
    auto movingBox = gameObjectManager.CreateGameObject(context);
    registry.emplace<Component::Sprite>(movingBox, TILE_TEXTURE, YELLOW, glm::vec2(16, 16), 9, 12, false, 6);
    transformSystem.SetLocalTransform(context, movingBox, glm::vec3(600.0f, 400.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    b2BodyDef kinematicBody3Def;
    kinematicBody3Def.type = b2_dynamicBody;
	b2PolygonShape groundBox;
	groundBox.SetAsBox(1 / 2.0f, 1 / 2.0f);
	b2PolygonShape groundShape;
    groundShape.SetAsBox(1 / 2.0f, 1 / 2.0f);
    physicsSystem.CreatePhysicsBody(context, movingBox, kinematicBody3Def, groundShape);
    auto& physicsBody3 = registry.get<Component::PhysicsBody>(movingBox);
    physicsBody3.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody3.syncToPhysics = true;     // Let transform drive physics

	DEBUG_INFO("Created Player Entity");

    resourcePool.CreateTexture(TILE_TEXTURE);
    resourcePool.LoadTextureInGPU(TILE_TEXTURE);

    resourcePool.CreateTexture(PLAYER_TEXTURE);
    resourcePool.LoadTextureInGPU(PLAYER_TEXTURE);
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
