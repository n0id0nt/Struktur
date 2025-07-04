#pragma once
#include "raylib.h"
#include "Engine/Core/GameContext.h"

namespace Struktur
{
	namespace System
	{
        class DebugSystem : public Core::ISystem
        {
        public:
            void Update(Core::GameContext& context) override
            {
                ::DrawFPS(10,10);
            }
        };
    }
}