#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class SoundSystem : public ISystem
        {
        public:         
            void Update(GameContext& context) override;
        };
    }
}
