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
			double pausedTime = 0.0f; // TODO is this the best way to handle this maybe flip it to the opposite (Will also better handle slomo)
			double deltaTime = 0.0f;
			double gameTime = 0.0f;
			double startTime = 0.0f;
			int gameWidth = 0;
			int gameHeight = 0;
			int applicationWidth = 0;
			int applicationHeight = 0;
			bool isFullScreen = false;
			int Loops = 0;
			GameState gameState = GameState::SPLASH_SCREEN;

			unsigned int fps = 60;
			float timeStep = 1.0f / fps;
			int velocityIterations = 16;
			int positionIterations = 10;
			float pixelsPerMeter = 64.f;
			const char* projectName = "Struktur";
			const char* inputBindingsPath = "assets/Settings/InputBindings/InputBindings.xml";
		};
	}
}
