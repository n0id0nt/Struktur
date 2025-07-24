#pragma once

#include "Engine/ECS/SystemManager.h"
#include "Engine/Game/StateManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class GameplaySystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };

        class GameplayRenderSystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };
    }
}
