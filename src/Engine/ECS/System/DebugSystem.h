#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class DebugSystem : public ISystem
        {
        public:
            void Update(GameContext& context) override;
        };
    }
}
