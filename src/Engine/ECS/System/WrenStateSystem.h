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
            std::string Name() const override { return "Wren State System"; }
        };

        class WrenStateRenderSystem : public ISystem
        {        
        public:
            void Update(GameContext& context) override;
            std::string Name() const override { return "Wren State Render System"; }
        };
    }
}
