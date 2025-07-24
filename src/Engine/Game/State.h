#pragma once

#include <string>

namespace Struktur
{
    class GameContext;
    
	namespace GameResource
	{
		class IState
		{
        public:
            IState() = default;

            virtual void Enter(GameContext& context) = 0;
            virtual void Update(GameContext& context) = 0;
            virtual void Render(GameContext& context) = 0;
            virtual void Exit(GameContext& context) = 0;

            virtual std::string GetStateName() const = 0;
        };
    }
}