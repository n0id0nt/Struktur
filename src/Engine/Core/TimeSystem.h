#pragma once

#include <cstdint>

namespace Struktur::Core
{
struct TimeSystem
{
	float unscaledDelta = 0.0f;
	float scaledDelta   = 0.0f;
	double unscaledTime = 0.0;
	double scaledTime   = 0.0;
	float timeScale     = 1.0f;

   private:
	double m_scaleChangeUnscaledTime = 0.0;
	double m_scaleChangeScaledTime   = 0.0;
	// Frame delta measured via SDL3's performance counter (works on both platforms via SDL3's Emscripten
	// support) instead of raylib's GetFrameTime(), whose core/window state is never initialised here.
	uint64_t m_lastTicks = 0;
	bool m_hasLastTicks  = false;

   public:
	void SetTimeScale(float newScale);
	void Update();
};
}  // namespace Struktur::Core
