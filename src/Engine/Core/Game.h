#pragma once

namespace Struktur
{
	namespace Core
	{
		class GameContext;

		void Game();
		void LoadData(GameContext& context);
        void UpdateLoop(GameContext& context);
        void GameLoop(GameContext& context);
        void SplashScreenLoop(GameContext& context);
        void LoadingLoop(GameContext& context);
	}
};
