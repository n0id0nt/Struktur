#pragma once

#include "Engine/Core/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class SpriteRenderSystem : public Core::ISystem
        {
        public:
            void Update(GameContext& context) override;
        };
    }
}
