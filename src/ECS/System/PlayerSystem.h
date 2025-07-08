#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "ECS/Component/Transform.h"
#include "ECS/Component/Player.h"
#include "ECS/Component/PhysicsBody.h"
#include "Engine/Core/GameContext.h"

namespace Struktur
{
	namespace System
	{
        class PlayerSystem : public Core::ISystem
        {
        public:
            void Update(Core::GameContext& context) override
            {
                // player movement system
                Core::GameData& gameData = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                glm::vec2 inputDir = context.GetInput().GetInputAxis2("Move");
                auto view = registry.view<Component::Transform, Component::Player, Component::PhysicsBody>();
                for (auto [entity, transform, player, physicsBody] : view.each())
                {
                    b2Vec2 velecity = b2Vec2(inputDir.x *  player.speed, inputDir.y * -player.speed);
                    physicsBody.body->SetLinearVelocity(velecity);
                }
            }
        };
    }
}