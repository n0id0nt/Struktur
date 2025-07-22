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
#include "Engine/ECS/System/GameplaySystem.h"
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
    systemManager.AddUpdateSystem<System::GameplaySystem>();
    systemManager.AddUpdateSystem<System::HierarchySystem>();
    systemManager.AddUpdateSystem<System::TransformSystem>();
    systemManager.AddUpdateSystem<System::CameraSystem>();
    systemManager.AddUpdateSystem<System::PhysicsSystem>();
    systemManager.AddRenderSystem<System::SpriteRenderSystem>();
    systemManager.AddRenderSystem<System::DebugSystem>();

    FileLoading::LevelParser::World world = FileLoading::LevelParser::LoadWorldMap(context, "assets/", "Levels/ExampleLDKTLevel.ldtk"); // need to sore the world somewhere to be refered to later.
    
    FileLoading::LevelParser::Level& firstLevel = world.levels[0]; // should probably actually store the first level somewhere
    GameResource::Level::LoadLevelEntities(context, firstLevel);

    gameObjectManager.CreateDeleteObjectCallBack(context);

    input.LoadInputBindings("assets/Settings/InputBindings/InputBindings.xml");

    resourcePool.CreateTexture(TILE_TEXTURE);
    resourcePool.LoadTextureInGPU(TILE_TEXTURE);

    resourcePool.CreateTexture(PLAYER_TEXTURE);
    resourcePool.LoadTextureInGPU(PLAYER_TEXTURE);
    DEBUG_INFO("Game Data Loaded");
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
