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
            double deltaTime = 0.0f;
            double gameTime = 0.0f;
            double startTime = 0.0f;
            int gameWidth = 0;
            int gameHeight = 0;
            int applicationWidth = 0;
            int applicationHeight = 0;
            int Loops = 0;
            GameState gameState = GameState::SPLASH_SCREEN;
        };
	}
}
