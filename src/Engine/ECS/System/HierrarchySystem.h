#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/Core/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class HierarchySystem : public Core::ISystem
        {
        public:
            HierarchySystem() {}

            void Update(GameContext& context) override {} // Empty Update loop 

            void SetParent(GameContext& context, entt::entity child, entt::entity parent);
            void RemoveFromParent(GameContext& context, entt::entity child, entt::entity parent);
            void DestroyEntity(GameContext& context, entt::entity entity);
        };
    }
}
