#pragma once

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

            void CreateDeleteObjectCallBack(GameContext& context);

            entt::entity CreateGameObject(GameContext& context, entt::entity parent = entt::null);

            void DestroyGameObject(GameContext& context, entt::entity entity);

        private:
            void OnChildrenDestroy(entt::registry& reg, entt::entity entity);
        };
    }
}
