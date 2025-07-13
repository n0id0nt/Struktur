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
        class DebugSystem : public Core::ISystem
        {
        public:
            void Update(Core::GameContext& context) override;
        };
    }
}