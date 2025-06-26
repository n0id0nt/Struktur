#include "raylib.h"

// Include all the libraries to verify they work
#include "entt/entt.hpp"           // Entity Component System
#include "glm/glm.hpp"             // Math library
#include "glm/gtc/matrix_transform.hpp"
#include "nlohmann/json.hpp"       // JSON handling
#include "pugixml.hpp"             // XML handling

extern "C" {
#include "lua.h"                   // Lua C API
#include "lauxlib.h"
#include "lualib.h"
}

// RayGUI is included differently
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                // Immediate mode GUI for RayLib

// Box2D
#include "box2d/box2d.h"          // 2D Physics

// ImGui with RayLib integration
#include "imgui.h"
#include "rlImGui.h"              // Official RayLib ImGui integration

#include <iostream>

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

// Game state structure with ECS
struct GameState {
    Vector2 playerPosition;
    Texture2D playerTexture;
    bool gameInitialized;
    
    // ECS World
    entt::registry registry;
    
    // Physics world
    b2World* physicsWorld;
    
    // ImGui state
    bool showDemoWindow;
    bool showLibraryInfo;
};

// Components for ECS
struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

struct Renderable {
    Color color;
    int size;
};

GameState gameState = {0};

void InitializeLibraries() {
    std::cout << "Initializing libraries..." << std::endl;
    
    // Initialize physics world
    b2Vec2 gravity(0.0f, -10.0f);
    gameState.physicsWorld = new b2World(gravity);
    
    // Create some ECS entities
    auto entity1 = gameState.registry.create();
    gameState.registry.emplace<Position>(entity1, 100.0f, 100.0f);
    gameState.registry.emplace<Velocity>(entity1, 50.0f, 0.0f);
    gameState.registry.emplace<Renderable>(entity1, RED, 20);
    
    auto entity2 = gameState.registry.create();
    gameState.registry.emplace<Position>(entity2, 200.0f, 150.0f);
    gameState.registry.emplace<Velocity>(entity2, -30.0f, 25.0f);
    gameState.registry.emplace<Renderable>(entity2, BLUE, 15);
    
    // Test JSON
    nlohmann::json config = {
        {"player", {
            {"speed", 200.0f},
            {"health", 100}
        }},
        {"game", {
            {"title", "RayLib Multi-Library Template"},
            {"version", "1.0.0"}
        }}
    };
    std::cout << "JSON Config: " << config.dump(2) << std::endl;
    
    // Test Lua with C API (simple and reliable)
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    
    // Execute a simple Lua script
    const char* luaScript = "function greet(name) return 'Hello from Lua, ' .. name .. '!' end";
    if (luaL_dostring(L, luaScript) == LUA_OK) {
        // Call the Lua function
        lua_getglobal(L, "greet");
        lua_pushstring(L, "RayLib");
        if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
            const char* result = lua_tostring(L, -1);
            std::cout << "Lua says: " << result << std::endl;
            lua_pop(L, 1);
        }
    } else {
        std::cout << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    
    lua_close(L);
    
    // Test GLM
    glm::vec3 position(1.0f, 2.0f, 3.0f);
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    std::cout << "GLM transform test completed" << std::endl;
    
    // Initialize ImGui with rlImGui (Official RayLib integration)
    rlImGuiSetup(true);  // Back to the original function names
    
    // Configure ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Enable docking (v1.91.8-docking supports this)
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    gameState.showDemoWindow = false;
    gameState.showLibraryInfo = true;
    
    std::cout << "All libraries initialized successfully!" << std::endl;
}

void UpdateECS() {
    // Update ECS system - move entities
    auto view = gameState.registry.view<Position, Velocity>();
    
    for (auto entity : view) {
        auto& pos = view.get<Position>(entity);
        auto& vel = view.get<Velocity>(entity);
        
        pos.x += vel.dx * GetFrameTime();
        pos.y += vel.dy * GetFrameTime();
        
        // Bounce off screen edges
        if (pos.x < 0 || pos.x > GetScreenWidth()) {
            vel.dx = -vel.dx;
        }
        if (pos.y < 0 || pos.y > GetScreenHeight()) {
            vel.dy = -vel.dy;
        }
    }
}

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

    // Update ECS
    UpdateECS();
    
    // Update physics
    gameState.physicsWorld->Step(GetFrameTime(), 6, 2);

    // Start ImGui frame using rlImGui
    rlImGuiBegin();

    // Draw everything
    BeginDrawing();
    ClearBackground(DARKBLUE);
    
    // Draw player sprite
    DrawTexture(gameState.playerTexture, 
                (int)gameState.playerPosition.x, 
                (int)gameState.playerPosition.y, 
                WHITE);
    
    // Draw ECS entities
    auto view = gameState.registry.view<Position, Renderable>();
    for (auto entity : view) {
        auto& pos = view.get<Position>(entity);
        auto& render = view.get<Renderable>(entity);
        
        DrawCircle((int)pos.x, (int)pos.y, render.size, render.color);
    }
    
    // Draw instructions
    DrawText("Use ARROW KEYS or WASD to move!", 10, 10, 20, WHITE);
    DrawText("RayLib Multi-Library Template", 10, 40, 16, LIGHTGRAY);
    DrawText("Press TAB to toggle library info", 10, 70, 16, LIGHTGRAY);
    
    // RayGUI example
    if (GuiButton(Rectangle{ 10, 100, 120, 30 }, "RayGUI Button")) {
        gameState.showLibraryInfo = !gameState.showLibraryInfo;
    }
    
    // Handle ImGui toggle
    if (IsKeyPressed(KEY_TAB)) {
        gameState.showLibraryInfo = !gameState.showLibraryInfo;
    }
    
    EndDrawing();

    // ImGui rendering with rlImGui
    if (gameState.showLibraryInfo) {
        ImGui::Begin("Library Information", &gameState.showLibraryInfo);
        
        ImGui::Text("Integrated Libraries:");
        ImGui::Separator();
        ImGui::Text("• RayLib 5.5 - Graphics & Audio");
        ImGui::Text("• EnTT 3.12.2 - Entity Component System");
        ImGui::Text("• GLM 0.9.9.8 - OpenGL Mathematics");
        ImGui::Text("• Lua 5.4.6 - Scripting Language (C API)");
        ImGui::Text("• nlohmann/json 3.11.3 - JSON Parser");
        ImGui::Text("• PugiXML 1.14 - XML Parser");
        ImGui::Text("• RayGUI 4.0 - Immediate Mode GUI");
        ImGui::Text("• Box2D 2.4.1 - 2D Physics Engine");
        ImGui::Text("• Dear ImGui 1.91.8-docking - Advanced GUI");
        ImGui::Text("• rlImGui - Official RayLib ImGui Integration");
        
        ImGui::Separator();
        ImGui::Text("ECS Entities: %d", (int)gameState.registry.size());
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                   1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "✅ Docking Enabled!");
        ImGui::BulletText("Drag window titles to dock/undock");
        ImGui::BulletText("Right-click title bars for options");
        ImGui::BulletText("This window can be docked anywhere");
        
        if (ImGui::Button("Show ImGui Demo")) {
            gameState.showDemoWindow = true;
        }
        
        ImGui::End();
    }
    
    if (gameState.showDemoWindow) {
        ImGui::ShowDemoWindow(&gameState.showDemoWindow);
    }

    // End ImGui frame and render using rlImGui
    rlImGuiEnd();
}

int main() {
    // Initialize window
    const int screenWidth = 1024;
    const int screenHeight = 768;
    
    InitWindow(screenWidth, screenHeight, "RayLib Multi-Library Template");
    
    // Load player texture
    gameState.playerTexture = LoadTexture("assets/player.png");
    
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
    
    // Initialize all libraries
    InitializeLibraries();
    
    gameState.gameInitialized = true;
    
#ifdef PLATFORM_WEB
    // Web platform - use emscripten main loop
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    // Desktop platform - standard game loop
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif
    
    // Cleanup
    rlImGuiShutdown();  // Proper rlImGui cleanup
    
    delete gameState.physicsWorld;
    UnloadTexture(gameState.playerTexture);
    CloseWindow();
    
    return 0;
}