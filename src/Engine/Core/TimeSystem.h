#pragma once

namespace Struktur::Core
{
    struct TimeSystem
    {
        float  unscaledDelta = 0.0f;
        float  scaledDelta   = 0.0f;
        double unscaledTime  = 0.0;
        double scaledTime    = 0.0;
        float  timeScale     = 1.0f;

    private:
        double m_scaleChangeUnscaledTime = 0.0;
        double m_scaleChangeScaledTime   = 0.0;

    public:
        void SetTimeScale(float newScale);
        void Update();
    };
}
