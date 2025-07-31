#pragma once

#include "glm/glm.hpp"
#include "entt/entt.hpp"

namespace Struktur
{
    class GameContext;

    namespace Player
    {
        void Create(GameContext& context, entt::entity entity);

        void PlayerControl(GameContext& context, entt::entity entity, glm::vec2 dir);

        void Interact(GameContext& context, entt::entity entity, entt::entity targetEntity);
        entt::entity CanInteract(GameContext& context, entt::entity entity);
    }
}
