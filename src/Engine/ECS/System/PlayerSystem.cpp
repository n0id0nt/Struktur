#include "PlayerSystem.h"

#include <cstdlib>
//#include <ctime>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/GameContext.h"

#include "engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"

void Struktur::System::PlayerSystem::Update(GameContext &context)
{
    auto& input = context.GetInput();
    auto& gameObjectManager = context.GetGameObjectManager();
    // player movement system
    Core::GameData& gameData = context.GetGameData();
    entt::registry& registry = context.GetRegistry();
    glm::vec2 inputDir = input.GetInputAxis2("Move");
    bool inputAddObject = input.IsInputJustPressed("AddObject");
    bool inputAddChild = input.IsInputJustPressed("AddChild");
    bool inputDeleteObject = input.IsInputJustPressed("DeleteObject");
    auto view = registry.view<Component::Transform, Component::Player, Component::PhysicsBody>();
    for (auto [entity, transform, player, physicsBody] : view.each())
    {
        b2Vec2 velecity = b2Vec2(inputDir.x *  player.speed, inputDir.y * -player.speed);
        physicsBody.body->SetLinearVelocity(velecity);
        if (inputAddObject)
        {
            //std::srand(std::time({}));
            auto child = gameObjectManager.CreateGameObject(context, entity);
            registry.emplace<Component::Sprite>(child, "assets/Circle.png", PINK, glm::vec2(8, 8), 2, 2, false, 1);
            auto& childTransform = registry.get<Component::Transform>(child);
            childTransform.position = { (float)(std::rand() % 200) - 100.0f, (float)(std::rand() % 200) - 100.0f, 0.0f }; // Relative to parent
            DEBUG_INFO("Add game object");
        }
        if (inputAddChild)
        {
            Component::Children* children = registry.try_get<Component::Children>(entity);
            if (children)
            {
                if (children->entities.size())
                {
                    entt::entity parent = children->entities[std::rand() % children->entities.size()];
                    //std::srand(std::time({}));
                    auto child = gameObjectManager.CreateGameObject(context, parent);
                    registry.emplace<Component::Sprite>(child, "assets/Circle.png", PURPLE, glm::vec2(8, 8), 2, 2, false, 1);
                    auto& childTransform = registry.get<Component::Transform>(child);
                    childTransform.position = { (float)(std::rand() % 200) - 100.0f, (float)(std::rand() % 200) - 100.0f, 0.0f }; // Relative to parent
                    DEBUG_INFO("Add child game object");

                    System::SystemManager& systemManager = context.GetSystemManager();
                    System::PhysicsSystem& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();

                    b2BodyDef kinematicBodyDef;
                    kinematicBodyDef.type = b2_dynamicBody;
                    physicsSystem.CreatePhysicsBody(context, child, kinematicBodyDef);
                    auto& physicsBody = registry.get<Component::PhysicsBody>(child);
                    physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
                    physicsBody.syncToPhysics = true;     // Let transform drive physics
                }
            }
        }
        if (inputDeleteObject)
        {
            Component::Children* children = registry.try_get<Component::Children>(entity);
            if (children)
            {
                if (children->entities.size())
                {
                    gameObjectManager.DestroyGameObject(context, children->entities[0]);
                    DEBUG_INFO("Delete game object");
                }
            }
        }
    }
}
