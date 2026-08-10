#include "TimeSystem.h"

#include <SDL3/SDL.h>

void Struktur::Core::TimeSystem::SetTimeScale(float newScale)
{
	m_scaleChangeScaledTime   = scaledTime;
	m_scaleChangeUnscaledTime = unscaledTime;
	timeScale                 = newScale;
}

void Struktur::Core::TimeSystem::Update()
{
	uint64_t nowTicks = SDL_GetPerformanceCounter();
	double rawDelta   = 0.0;
	if (m_hasLastTicks)
	{
		rawDelta = static_cast<double>(nowTicks - m_lastTicks) / static_cast<double>(SDL_GetPerformanceFrequency());
	}
	m_lastTicks    = nowTicks;
	m_hasLastTicks = true;

	unscaledDelta = static_cast<float>(rawDelta);
	unscaledTime += rawDelta;

	scaledDelta = unscaledDelta * timeScale;
	scaledTime  = m_scaleChangeScaledTime + (unscaledTime - m_scaleChangeUnscaledTime) * static_cast<double>(timeScale);
}
