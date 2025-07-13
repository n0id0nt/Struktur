#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"



namespace Struktur
{
    namespace Core
    {
        class GameContext;
    }
	namespace System
	{
        class GameObjectManager
        {
        public:
            GameObjectManager() {}

            void CreateDeleteObjectCallBack(Core::GameContext& context);

            entt::entity CreateGameObject(Core::GameContext& context, entt::entity parent = entt::null);

            void DestroyGameObject(Core::GameContext& context, entt::entity entity);

        private:
            void OnChildrenDestroy(entt::registry& reg, entt::entity entity);
        };
    }
}