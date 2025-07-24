#pragma once

namespace Struktur
{
	class GameContext;

	void Game();
	void UpdateLoop(void* userData);
	void InitialiseGame(GameContext& context);
	void ExitGame(GameContext& context);
	void GameLoop(GameContext& context);
	void SplashScreenLoop(GameContext& context);
	void LoadingLoop(GameContext& context);
};
