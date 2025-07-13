#pragma once

#include "Engine/Core/SystemManager.h"

namespace Struktur
{
    namespace Core
    {
        class GameContext;
    }
	namespace System
	{
        class SpriteRenderSystem : public Core::ISystem
        {
        public:
            void Update(Core::GameContext& context) override;
        };
    }
}
