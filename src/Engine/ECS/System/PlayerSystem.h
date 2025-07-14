#pragma once

#include "Engine/Core/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class PlayerSystem : public Core::ISystem
        {        
        public:
            void Update(GameContext& context) override;
        };
    }
}
