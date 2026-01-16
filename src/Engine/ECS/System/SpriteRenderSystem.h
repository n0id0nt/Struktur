#pragma once

#include "entt/entt.hpp"

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

    namespace Component
    {
        struct Sprite;
        struct WorldTransform;
    }

	namespace System
	{
        class SpriteRenderSystem : public ISystem
        {
        private:
            struct SpriteRenderData
            {
                entt::entity entity;
                Component::Sprite* sprite;
                Component::WorldTransform* worldTransform;
                int renderPriority;
            };

        public:
            void Update(GameContext& context) override;
            std::string Name() const override { return "Sprite Render System"; }

        private:
            std::vector<SpriteRenderData> m_spritesToRender;
        };
    }
}
