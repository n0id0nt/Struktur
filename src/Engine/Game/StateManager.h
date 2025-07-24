#pragma once

#include <memory>
#include <string>

#include "Engine/Game/State.h"

namespace Struktur
{
    class GameContext;
    
	namespace GameResource
	{
        class IState;

		class StateManager
		{
        public:
            StateManager() = default;

            void ChangeState(GameContext& context, std::unique_ptr<IState> newState);

            void Update(GameContext& context);
            void Render(GameContext& context);

            void ReleaseState(GameContext& context);

            std::string GetCurrentStateName();
            IState* GetCurrentState();

        private:
            std::unique_ptr<IState> m_state;
        };
    }
}