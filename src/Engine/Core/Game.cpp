#include "Game.h"

#include "raylib.h"
#include "raymath.h"

#include <string>
#include "Engine/Core/GameContext.h"
#include "Engine/Core/Input.h"


constexpr static const unsigned int FPS = 60;
constexpr static const float TIME_STEP = 1.0f / FPS;
constexpr static const int VELOCITY_ITERATIONS = 6;
constexpr static const int POSITION_ITERATIONS = 4;

enum class GameState{
    SplashScreen,
    Loading,
    Game,

    Count
};

GameState gameState = GameState::SplashScreen;
bool shouldQuit = false;
double startTime = 0;

void Struktur::Core::LoadData(GameContext& context)
{
    Input& input = context.GetInput();
    input.LoadInputBindings("assets/Settings/InputBindings/InputBindings.xml");
    TraceLog(LOG_INFO, "Game Data Loaded");
}

void Struktur::Core::SplashScreenLoop(GameContext& context)
{
    //fade in time
    const double fadeInTime = 1.5;
    const double holdTime = 1;
    const double fadeOutTime = 1.5;
    const double currentTime = GetTime();
    if (currentTime > startTime + fadeInTime + holdTime + fadeOutTime)
    {
        gameState = GameState::Game;
        TraceLog(LOG_INFO, "Start Game Loop");
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
    int fontWidth = MeasureText(splashScreenName.c_str(), fontSize);
    int width = GetScreenWidth();
    int height = GetScreenHeight();

    BeginDrawing();
    ClearBackground(Color{ 0,0,0,255 });
    DrawText(splashScreenName.c_str(), (width - fontWidth) / 2.f, (height - fontSize) / 2.f, fontSize, Color{ 255,255,255,(unsigned char)textAlpha });
    EndDrawing();
}

void Struktur::Core::LoadingLoop(GameContext& context)
{

}

float shapePosX = 500.f;
float shapePosY = 350.f;
float speed = 30.f;

void Struktur::Core::GameLoop(GameContext& context)
{
    if (WindowShouldClose()) 
    {
        shouldQuit = true;
    }

    glm::vec2 inputDir = context.GetInput().GetInputAxis2("Move");

    shapePosX += GetFrameTime() * inputDir.x * speed;
    shapePosY += GetFrameTime() * inputDir.y * speed;

    BeginDrawing();
    ClearBackground(GREEN);

    DrawRectangle(shapePosX, shapePosY, 30, 30, BROWN);
    //Struktur::System::Render::Update(gameData.registry, gameData.resourcePool, gameData.camera);
    //Struktur::System::UI::Update(gameData.registry, gameData.resourcePool, gameData.dialogueText, systemTime, gameData.gameState, gameData.previousGameState, gameData.shouldQuit);
    //debug render(lines and stuff)
    //render UI
    //render debug UI
    //Render IMGUI (When i actually add this)
    EndDrawing();
}

void Struktur::Core::UpdateLoop(GameContext& context) {
    float dt = GetFrameTime();
    
    switch(gameState)
    {
    case GameState::SplashScreen:
        SplashScreenLoop(context);
        return;
    case GameState::Loading:
        LoadingLoop(context);
        return;
    case GameState::Game:
        GameLoop(context);
        return;
    }
}

void Struktur::Core::Game() {
    // Initialize window
    const int screenWidth = 1024;
    const int screenHeight = 768;

    GameContext context;
    
    InitWindow(screenWidth, screenHeight, "Struktur");
    SetExitKey(KEY_NULL);

    LoadData(context);

    // Load resources
    startTime = GetTime();
#ifdef PLATFORM_WEB
    // Web platform - use emscripten main loop
    emscripten_set_main_loop_arg(UpdateLoop, context, FPS, 1);
#else
    // Desktop platform - standard game loop
    SetTargetFPS(FPS);
    
    while (!WindowShouldClose()) {
        UpdateLoop(context);
    }
#endif
    
    // Cleanup
    CloseWindow();
}