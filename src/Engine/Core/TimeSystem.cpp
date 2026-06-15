#include "TimeSystem.h"

#include "raylib.h"

void Struktur::Core::TimeSystem::SetTimeScale(float newScale)
{
	m_scaleChangeScaledTime   = scaledTime;
	m_scaleChangeUnscaledTime = unscaledTime;
	timeScale                 = newScale;
}

void Struktur::Core::TimeSystem::Update()
{
	double rawDelta = static_cast<double>(::GetFrameTime());

	unscaledDelta = static_cast<float>(rawDelta);
	unscaledTime += rawDelta;

	scaledDelta = unscaledDelta * timeScale;
	scaledTime  = m_scaleChangeScaledTime + (unscaledTime - m_scaleChangeUnscaledTime) * static_cast<double>(timeScale);
}
