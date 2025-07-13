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
        class HierarchySystem : public Core::ISystem
        {
        public:
            HierarchySystem() {}

            void Update(Core::GameContext& context) override {} // Empty Update loop 

            void SetParent(Core::GameContext& context, entt::entity child, entt::entity parent);
            void RemoveFromParent(Core::GameContext& context, entt::entity child, entt::entity parent);
            void DestroyEntity(Core::GameContext& context, entt::entity entity);
        };
    }
}