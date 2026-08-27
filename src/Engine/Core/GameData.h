#pragma once

namespace Struktur
{
namespace Core
{
enum class GameState
{
	SPLASH_SCREEN,
	LOADING,  // Not currently used cause I don't have multithreading set up
	GAME,
	QUIT,

	COUNT
};

struct GameData
{
	int gameWidth         = 0;
	int gameHeight        = 0;
	int applicationWidth  = 0;
	int applicationHeight = 0;
	bool isFullScreen     = false;
	GameState gameState   = GameState::SPLASH_SCREEN;

	unsigned int fps        = 60;
	float timeStep          = 1.0f / fps;
	int velocityIterations  = 16;
	int positionIterations  = 10;
	float pixelsPerMeter    = 64.f;
	const char* projectName = "Struktur";

	// Fixed-update accumulator (see GameLoop in Game.cpp) - real elapsed time banked up between fixed steps.
	float physicsAccumulator = 0.0f;
	// Spiral-of-death guard: caps how many fixed steps can run in a single frame before the accumulator's
	// overflow is discarded instead of chased (matches Godot's own default of 8 - see physics/common/
	// max_physics_steps_per_frame).
	int maxFixedStepsPerFrame = 8;

	// Frame pacing (see GraphicsDevice::SetVSync and the frame-cap sleep in Game.cpp's desktop loop) - the live,
	// in-memory mirror of Core::EngineSettings, loaded once at boot and written back whenever changed.
	bool vsyncEnabled = true;
	int targetFps     = 0;  // 0 = uncapped
};
}  // namespace Core
}  // namespace Struktur
