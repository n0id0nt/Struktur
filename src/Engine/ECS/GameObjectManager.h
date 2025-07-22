#pragma once

#include <string>
#include "entt/entt.hpp"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class GameObjectManager
        {
        public:
            GameObjectManager() {}
            ~GameObjectManager();

            void CreateDeleteObjectCallBack(GameContext& context);

            entt::entity CreateGameObject(GameContext& context, const std::string& identifier, entt::entity parent = entt::null);

            void DestroyGameObject(GameContext& context, entt::entity entity);

        private:
            void OnChildrenDestroy(entt::registry& reg, entt::entity entity);
            void OnPhysicsBodyDestory(entt::registry& reg, entt::entity entity);

            GameContext* m_context = nullptr;
        };
    }
}
