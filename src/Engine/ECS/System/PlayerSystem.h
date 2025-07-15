#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class PlayerSystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };
    }
}
