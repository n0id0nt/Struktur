#pragma once

namespace Struktur
{
	namespace Core
	{
		enum class GameState
		{
			SPLASH_SCREEN,
			LOADING, // Not currently used cause I don't have multithreading set up
			GAME,
			QUIT,

			COUNT
		};

		struct GameData
		{
			int gameWidth = 0;
			int gameHeight = 0;
			int applicationWidth = 0;
			int applicationHeight = 0;
			bool isFullScreen = false;
			GameState gameState = GameState::SPLASH_SCREEN;

			unsigned int fps = 60;
			float timeStep = 1.0f / fps;
			int velocityIterations = 16;
			int positionIterations = 10;
			float pixelsPerMeter = 64.f;
			const char* projectName = "Struktur";
		};
	}
}
