#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class WrenStateSystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };

        class WrenStateRenderSystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };
    }
}
