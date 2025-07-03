#pragma once

namespace Struktur
{
	namespace Core
	{
        enum class GameState{
            SPLASH_SCREEN,
            LOADING,
            GAME,
            QUIT,
        
            COUNT
        };

		struct GameData
        {
            double pausedTime = 0.0f;
            double dt = 0.0f;
            double gameTime = 0.0f;
            double startTime = 0.0f;
            int screenWidth = 0;
            int screenHeight = 0;
            GameState gameState = GameState::SPLASH_SCREEN;
        };
	};
};