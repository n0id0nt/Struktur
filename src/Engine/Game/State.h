#pragma once

#include <string>

namespace Struktur
{
    class GameContext;
    
	namespace GameResource
	{
        class StateManager;

		class IState
		{
        public:
            IState() = default;

            virtual void Enter(GameContext& context, StateManager& stateManager) = 0;
            virtual void Update(GameContext& context, StateManager& stateManager) = 0;
            virtual void Render(GameContext& context, StateManager& stateManager) = 0;
            virtual void Exit(GameContext& context, StateManager& stateManager) = 0;

            virtual std::string GetStateName() const = 0;
        };
    }
}