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
        class PlayerSystem : public Core::ISystem
        {        
        public:
            void Update(Core::GameContext& context) override;
        };
    }
}