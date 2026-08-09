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
#if !defined(PLATFORM_WEB)
	// Desktop measures its own frame delta via SDL3's performance counter instead of raylib's GetFrameTime() -
	// raylib's core/window state (that function's source) is never initialised here, bgfx/SDL3 own that now.
	uint64_t m_lastTicks  = 0;
	bool m_hasLastTicks   = false;
#endif

   public:
	void SetTimeScale(float newScale);
	void Update();
};
}  // namespace Struktur::Core
