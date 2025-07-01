#pragma once

namespace Struktur
{
	namespace Core
	{
		struct GameData
        {
            bool shouldQuit;
            float pausedTime = 0.0f;
            //GameState gameState = GameState::MAIN_MENU;
            //GameState previousGameState = GameState::PAUSE;
        };
	};
};