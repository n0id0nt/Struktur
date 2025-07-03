#pragma once

namespace Struktur
{
	namespace Core
	{
		class GameContext;

		void Game();
        void UpdateLoop(void* userData);
		void LoadData(GameContext& context);
        void GameLoop(GameContext& context);
        void SplashScreenLoop(GameContext& context);
        void LoadingLoop(GameContext& context);
	}
};
