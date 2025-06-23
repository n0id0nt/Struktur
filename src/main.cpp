#include "raylib.h"
#include <iostream>

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

// used to get web width and height in browser
#if defined(PLATFORM_WEB)
EM_JS(int, getBrowserWidth, (), {
    return window.innerWidth;
});

EM_JS(int, getBrowserHeight, (), {
    return window.innerHeight;
});
#endif

// Game state structure
struct GameState {
    Vector2 playerPosition;
    Texture2D playerTexture;
    bool gameInitialized;
};

GameState gameState = {0};

void UpdateDrawFrame() {
    // Handle input
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        gameState.playerPosition.x += 200.0f * GetFrameTime();
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        gameState.playerPosition.x -= 200.0f * GetFrameTime();
    }
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        gameState.playerPosition.y -= 200.0f * GetFrameTime();
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        gameState.playerPosition.y += 200.0f * GetFrameTime();
    }

    // Keep player on screen
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if (gameState.playerPosition.x < 0) gameState.playerPosition.x = 0;
    if (gameState.playerPosition.x > screenWidth - gameState.playerTexture.width) {
        gameState.playerPosition.x = screenWidth - gameState.playerTexture.width;
    }
    if (gameState.playerPosition.y < 0) gameState.playerPosition.y = 0;
    if (gameState.playerPosition.y > screenHeight - gameState.playerTexture.height) {
        gameState.playerPosition.y = screenHeight - gameState.playerTexture.height;
    }

    // Draw everything
    BeginDrawing();
    ClearBackground(DARKBLUE);
    
    // Draw player sprite
    DrawTexture(gameState.playerTexture, 
                (int)gameState.playerPosition.x, 
                (int)gameState.playerPosition.y, 
                WHITE);
    
    // Draw instructions
    DrawText("Use ARROW KEYS or WASD to move!", 10, 10, 20, WHITE);
    DrawText("RayLib Cross-Platform Template", 10, 40, 16, LIGHTGRAY);
    
    EndDrawing();
}

int main() {
    // Initialize window
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    InitWindow(screenWidth, screenHeight, "RayLib Cross-Platform Game");
    
    // Load player texture
    gameState.playerTexture = LoadTexture("assets/Circle.png");
    
    // Check if texture loaded successfully
    if (gameState.playerTexture.id == 0) {
        std::cout << "Failed to load player texture!" << std::endl;
        // Create a simple colored rectangle as fallback
        Image img = GenImageColor(32, 32, RED);
        gameState.playerTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    
    // Initialize player position (center of screen)
    gameState.playerPosition.x = screenWidth / 2.0f - gameState.playerTexture.width / 2.0f;
    gameState.playerPosition.y = screenHeight / 2.0f - gameState.playerTexture.height / 2.0f;
    
    gameState.gameInitialized = true;
    
#ifdef PLATFORM_WEB
    // Web platform - use emscripten main loop
    emscripten_set_main_loop(UpdateDrawFrame, 60, true);
#else
    // Desktop platform - standard game loop
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif
    
    // Cleanup
    UnloadTexture(gameState.playerTexture);
    CloseWindow();
    
    return 0;
}