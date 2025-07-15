#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class SpriteRenderSystem : public ISystem
        {
        public:
            void Update(GameContext& context) override;

        private:
            struct spriteDraw {
                Texture2D texture;
                Rectangle sourceRect;
                Rectangle destRect;
                Vector2 offset;
                float angle;
            };
        };
    }
}
