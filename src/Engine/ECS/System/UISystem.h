#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class UISystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };

        class UIRenderSystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };
    }
}