#pragma once

#include "glm/glm.hpp"

namespace Struktur
{
	namespace Component
	{
        struct Camera
        {
            int cameraPriority = 0;

            float zoom = 1.f;
            float angle = 0.f;

            glm::vec2 offset;
            glm::vec2 deadZone;

            glm::vec2 damping;

            bool forcePosition = false;
            float traumaTime = 1.f;
            float maxOffset = 0.6f;
            float maxAngle = 1.f;
            float trauma = 0.f;
            float shakeAmplitude = 0.1f;
        };
    }
}