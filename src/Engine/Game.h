#pragma once

namespace Struktur
{
	class GameContext;

	void Game();
	void UpdateLoop(void* userData);
	void LoadData(GameContext& context);
	void GameLoop(GameContext& context);
	void SplashScreenLoop(GameContext& context);
	void LoadingLoop(GameContext& context);
};
