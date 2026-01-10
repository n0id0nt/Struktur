#pragma once

namespace Struktur
{
	class GameContext;

	void Game();
	void SplashScreenUpdateLoop(void* userData);
	void MainUpdateLoop(void* userData);
	void InitialiseGame(GameContext& context);
	void ExitGame(GameContext& context);
	void GameLoop(GameContext& context);
	void SplashScreenLoop(GameContext& context);
#ifdef DEBUG
	void StartDebugGame(GameContext& context);
	void StopDebugGame(GameContext& context);
	void ClearGameSystems(GameContext& context);
	void StartGameSystems(GameContext& context);
#endif
};
