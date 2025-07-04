#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "ECS/Component/Transform.h"
#include "ECS/Component/Player.h"
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
                auto view = registry.view<Component::Transform, Component::Player>();
                for (auto [entity, transform, player] : view.each())
                {
                    transform.position.x += inputDir.x * gameData.dt * player.speed;
                    transform.position.y += inputDir.y * gameData.dt * player.speed;
                }
            }
        };
    }
}