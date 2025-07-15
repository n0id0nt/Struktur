#pragma once
#include "glm/glm.hpp"
#include "entt/entt.hpp"

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class TransformSystem : public ISystem
        {
        public:

            void Update(GameContext& context) override;

            glm::vec3 WorldToLocal(GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity);
            float GetWorldRotation(GameContext& context, entt::entity entity);

        private:
            void UpdateWorldTransform(GameContext& context, entt::entity entity, const glm::mat4& parentMatrix);
        };
    }
}
