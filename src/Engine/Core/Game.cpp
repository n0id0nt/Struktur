#include "Game.h"

#include <string>
#include <memory>
#include <variant>
#include "raylib.h"
#include "raymath.h"
#include "box2d/box2d.h"
#ifdef PLATFORM_WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "Debug/Assertions.h"
#include "Engine/Core/GameContext.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/SystemManager.h"
#include "ECS/Component/Transform.h"
#include "ECS/System/GameObjectManager.h"

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

struct PhysicsBody {
	b2Body* body = nullptr;
	bool isKinematic = false;
	bool syncFromPhysics = true;
	bool syncToPhysics = false;
};

struct PhysicsWorld {
    std::unique_ptr<b2World> world;
    float timeStep = TIME_STEP;
    int32 velocityIterations = VELOCITY_ITERATIONS;
    int32 positionIterations = POSITION_ITERATIONS;
};

class PlayerSystem : public Struktur::Core::ISystem
{
public:
    void Update(Struktur::Core::GameContext& context) override
    {
        // player movement system
        Struktur::Core::GameData& gameData = context.GetGameData();
        entt::registry& registry = context.GetRegistry();
        glm::vec2 inputDir = context.GetInput().GetInputAxis2("Move");
        auto view = registry.view<Struktur::Component::Transform, Player>();
        for (auto [entity, transform, player] : view.each())
        {
            transform.position.x += inputDir.x * gameData.dt * player.speed;
            transform.position.y += inputDir.y * gameData.dt * player.speed;
        }
    }
};

class RenderSystem : public Struktur::Core::ISystem
{
public:
    void Update(Struktur::Core::GameContext& context) override
    {
        Struktur::Core::ResourcePool& resourcePool = context.GetResourcePool();
        entt::registry& registry = context.GetRegistry();
        auto view = registry.view<Struktur::Component::WorldTransform>();
        for (auto [entity, worldTransform] : view.each())
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
    }
};

class DubugSystem : public Struktur::Core::ISystem
{
public:
    void Update(Struktur::Core::GameContext& context) override
    {
        ::DrawFPS(10,10);
    }
};

class PhysicsSystem : public Struktur::Core::ISystem
{
private:
    std::unique_ptr<b2World> m_physicsWorld;

public:
    PhysicsSystem()
    {
        m_physicsWorld = std::make_unique<b2World>(b2Vec2_zero);
    }
    
    void Update(Struktur::Core::GameContext& context) override
    {
        stepPhysics(context, TIME_STEP);
    }

    void stepPhysics(Struktur::Core::GameContext& context, float deltaTime) 
    {
        syncTransformsToPhysics(context);

        m_physicsWorld->Step(deltaTime, 6, 2);
        
        // Update transforms from physics for dynamic bodies
        syncPhysicsToTransforms(context);
    }

    void syncPhysicsToTransforms(Struktur::Core::GameContext& context) 
    {
        entt::registry& registry = context.GetRegistry();

        auto view = registry.view<PhysicsBody, Struktur::Component::Transform>();
        
        for (auto entity : view) 
        {
            auto& physicsBody = view.get<PhysicsBody>(entity);
            auto& transform = view.get<Struktur::Component::Transform>(entity);
            
            if (physicsBody.body && physicsBody.syncFromPhysics) 
            {
                // Get world position from physics
                b2Vec2 position = physicsBody.body->GetPosition();
                float angle = physicsBody.body->GetAngle();
                
                // Convert to local space if entity has a parent
                if (auto* parent = registry.try_get<Struktur::Component::Parent>(entity)) 
                {
                    if (parent->entity != entt::null) {
                        glm::vec3 worldPos(position.x, position.y, 0.0f);
                        glm::vec3 localPos = worldToLocal(context, worldPos, parent->entity);
                        transform.position = localPos;
                        
                        // Handle rotation relative to parent
                        float parentAngle = getWorldRotation(context, parent->entity);
                        float localAngle = angle - parentAngle;
                        transform.rotation = glm::angleAxis(localAngle, glm::vec3(0, 0, 1));
                    } else {
                        // No parent, use world coordinates directly
                        transform.position = glm::vec3(position.x, position.y, 0.0f);
                        transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                    }
                } 
                else 
                {
                    // No parent, use world coordinates directly
                    transform.position = glm::vec3(position.x, position.y, 0.0f);
                    transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                }
            }
        }
    }

    void syncTransformsToPhysics(Struktur::Core::GameContext& context) 
    {
        entt::registry& registry = context.GetRegistry();
        auto view = registry.view<PhysicsBody, Struktur::Component::Transform, Struktur::Component::WorldTransform>();
        
        for (auto entity : view) 
        {
            auto& physicsBody = view.get<PhysicsBody>(entity);
            auto& worldTransform = view.get<Struktur::Component::WorldTransform>(entity);
            
            if (physicsBody.body && physicsBody.syncToPhysics) 
            {
                // Extract position and rotation from world transform matrix
                glm::vec3 worldPos = glm::vec3(worldTransform.matrix[3]);
                
                // For 2D, extract rotation from matrix
                float angle = atan2(worldTransform.matrix[1][0], worldTransform.matrix[0][0]);
                
                physicsBody.body->SetTransform(b2Vec2(worldPos.x, worldPos.y), angle);
            }
        }
    }

    b2Body* createPhysicsBody(Struktur::Core::GameContext& context, entt::entity entity, const b2BodyDef& bodyDef) 
    {
        entt::registry& registry = context.GetRegistry();
        b2Body* body = m_physicsWorld->CreateBody(&bodyDef);
        body->GetUserData().pointer = static_cast<uintptr_t>(entity);
        
        registry.emplace<PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);
        
        return body;
    }
    
private:
    glm::vec3 worldToLocal(Struktur::Core::GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity) 
    {
        entt::registry& registry = context.GetRegistry();
        if (auto* parentWorld = registry.try_get<Struktur::Component::WorldTransform>(parentEntity)) 
        {
            glm::mat4 parentInverse = glm::inverse(parentWorld->matrix);
            glm::vec4 localPos = parentInverse * glm::vec4(worldPos, 1.0f);
            return glm::vec3(localPos);
        }
        return worldPos;
    }

    float getWorldRotation(Struktur::Core::GameContext& context, entt::entity entity) 
    {
        entt::registry& registry = context.GetRegistry();
        if (auto* worldTransform = registry.try_get<Struktur::Component::WorldTransform>(entity)) 
        {
            return atan2(worldTransform->matrix[1][0], worldTransform->matrix[0][0]);
        }
        return 0.0f;
    }
};

void Struktur::Core::LoadData(GameContext& context)
{
    Input& input = context.GetInput();
    entt::registry& registry = context.GetRegistry();
    ResourcePool& resourcePool = context.GetResourcePool();
    SystemManager& systemManager = context.GetSystemManager();

    systemManager.AddUpdateSystem<PlayerSystem>();
    System::GameObjectManager& gameObjectManager = systemManager.AddUpdateSystem<System::GameObjectManager>(context);
    PhysicsSystem& physicsSystem = systemManager.AddUpdateSystem<PhysicsSystem>();
    systemManager.AddRenderSystem<RenderSystem>();
    systemManager.AddRenderSystem<DubugSystem>();

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
    b2BodyDef kinematicBodyDef;
    kinematicBodyDef.type = b2_kinematicBody;
    physicsSystem.createPhysicsBody(context, child2, kinematicBodyDef);
    auto& physicsBody = registry.get<PhysicsBody>(child2);
    physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody.syncToPhysics = true;     // Let transform drive physics

    auto wall = gameObjectManager.CreateGameObject(context);
    auto& wallTransform = registry.get<Component::Transform>(wall);
    wallTransform.position = {300.0f, 200.0f, 0.0f};
    b2BodyDef kinematicBody2Def;
    physicsSystem.createPhysicsBody(context, wall, kinematicBody2Def);
    auto& physicsBody2 = registry.get<PhysicsBody>(wall);
    physicsBody2.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody2.syncToPhysics = true;     // Let transform drive physics

    auto movingBox = gameObjectManager.CreateGameObject(context);
    auto& movingBoxTransform = registry.get<Component::Transform>(movingBox);
    movingBoxTransform.position = {700.0f, 700.0f, 0.0f};
    b2BodyDef kinematicBody3Def;
    kinematicBody3Def.type = b2_kinematicBody;
    physicsSystem.createPhysicsBody(context, movingBox, kinematicBody3Def);
    auto& physicsBody3 = registry.get<PhysicsBody>(movingBox);
    physicsBody3.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody3.syncToPhysics = true;     // Let transform drive physics


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

    systemManager.Update(context);    
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