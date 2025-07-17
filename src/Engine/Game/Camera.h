#pragma once

#include "raylib.h"
#include "glm/glm.hpp"

namespace Struktur
{
	namespace GameResource
	{
		struct Camera
		{
            glm::vec2 offset;
            glm::vec2 target;
            float rotation;
            float zoom;
			glm::vec2 previousCameraPosition;
			float previousCameraAngle;

			glm::vec2 WorldPosToScreenPos(glm::vec2 worldPos);
			glm::vec2 ScreenPosToWorldPos(glm::vec2 screenPos);
            ::Camera2D GetRaylibCamera();
		};
	}
}
