#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "ECS/Component/Transform.h"
#include "ECS/Component/Player.h"
#include "ECS/Component/Sprite.h"
#include "Engine/Core/GameContext.h"

namespace Struktur
{
	namespace System
	{
        class SpriteRenderSystem : public Core::ISystem
        {
        public:
            void Update(Struktur::Core::GameContext& context) override
            {
                Struktur::Core::ResourcePool& resourcePool = context.GetResourcePool();
                entt::registry& registry = context.GetRegistry();
                auto view = registry.view<Component::Sprite, Component::WorldTransform>();
                for (auto [entity, sprite, worldTransform] : view.each())
                {
                    glm::vec3 worldPos = glm::vec3(worldTransform.matrix[3]);
            
                    ::DrawTexture(resourcePool.RetrieveTexture(sprite.fileName), worldPos.x, worldPos.y, sprite.color);
                }
            }
        };
    }
}