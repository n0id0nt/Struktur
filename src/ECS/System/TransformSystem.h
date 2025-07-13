#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/Core/SystemManager.h"

namespace Struktur
{
    namespace Core
    {
        class GameContext;
    }
	namespace System
	{
        class TransformSystem : public Core::ISystem
        {
        public:

            void Update(Core::GameContext& context) override;

        private:
            void UpdateWorldTransform(Core::GameContext& context, entt::entity entity, const glm::mat4& parentMatrix);
        };
    }
}
