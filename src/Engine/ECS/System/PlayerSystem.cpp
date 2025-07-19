#include "PlayerSystem.h"

#include <cstdlib>
//#include <ctime>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/GameContext.h"

#include "engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"

void Struktur::System::PlayerSystem::Update(GameContext &context)
{
    auto& input = context.GetInput();
    auto& gameObjectManager = context.GetGameObjectManager();
	SystemManager& systemManager = context.GetSystemManager();
	TransformSystem& transformSystem = systemManager.GetSystem<TransformSystem>();

    // player movement system
    Core::GameData& gameData = context.GetGameData();
    entt::registry& registry = context.GetRegistry();
    glm::vec2 inputDir = input.GetInputAxis2("Move");
    bool inputAddObject = input.IsInputJustPressed("AddObject");
    bool inputAddChild = input.IsInputJustPressed("AddChild");
    bool inputDeleteObject = input.IsInputJustPressed("DeleteObject");
    auto view = registry.view<Component::LocalTransform, Component::Player, Component::PhysicsBody>();
    for (auto [entity, transform, player, physicsBody] : view.each())
    {
        b2Vec2 velecity = b2Vec2(inputDir.x *  player.speed, inputDir.y * -player.speed);
        physicsBody.body->SetLinearVelocity(velecity);
        if (inputAddObject)
        {
            //std::srand(std::time({}));
            auto child = gameObjectManager.CreateGameObject(context, entity);
            registry.emplace<Component::Sprite>(child, "assets/Tiles/cavesofgallet_tiles.png", PINK, glm::vec2(8, 8), 20, 20, false, 10);
			transformSystem.SetLocalTransform(context, child, glm::vec3((float)(std::rand() % 200) - 100.0f, (float)(std::rand() % 200) - 100.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            DEBUG_INFO("Add game object");
        }
        if (inputAddChild)
        {
            Component::Children* children = registry.try_get<Component::Children>(entity);
            if (children)
            {
                if (!children->entities.empty())
                {
                    entt::entity parent = children->entities[std::rand() % children->entities.size()];
                    //std::srand(std::time({}));
                    auto child = gameObjectManager.CreateGameObject(context, parent);
                    registry.emplace<Component::Sprite>(child, "assets/Tiles/cavesofgallet_tiles.png", PURPLE, glm::vec2(8, 8), 20, 20, false, 11);
                    transformSystem.SetLocalTransform(context, child, glm::vec3((float)(std::rand() % 200) - 100.0f, (float)(std::rand() % 200) - 100.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                    DEBUG_INFO("Add child game object");

                    SystemManager& systemManager = context.GetSystemManager();
                    PhysicsSystem& physicsSystem = systemManager.GetSystem<PhysicsSystem>();

                    b2BodyDef kinematicBodyDef;
                    kinematicBodyDef.type = b2_dynamicBody;
					b2PolygonShape childShape;
                    childShape.SetAsBox(1 / 2.0f, 1 / 2.0f);
                    physicsSystem.CreatePhysicsBody(context, child, kinematicBodyDef, childShape);
                }
            }
        }
        if (inputDeleteObject)
        {
            Component::Children* children = registry.try_get<Component::Children>(entity);
            if (children)
            {
                if (!children->entities.empty())
                {
                    gameObjectManager.DestroyGameObject(context, children->entities[0]);
                    DEBUG_INFO("Delete game object");
                }
            }
        }
    }
}
