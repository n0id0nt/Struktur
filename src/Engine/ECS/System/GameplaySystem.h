#pragma once

#include "Engine/ECS/SystemManager.h"

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
    }
}
