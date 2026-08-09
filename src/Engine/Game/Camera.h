#pragma once

#include "glm/glm.hpp"

#if defined(PLATFORM_WEB)
	#include "raylib.h"
#endif

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

	glm::vec2 WorldPosToScreenPos(const glm::vec2& worldPos);
	glm::vec2 ScreenPosToWorldPos(const glm::vec2& screenPos);

	// Standard 2D camera view matrix: translate by -target, rotate, scale by zoom, translate by +offset.
	glm::mat4 GetViewMatrix() const;
	// Screen-space orthographic projection (origin top-left, Y down), matching raylib's prior screen convention.
	glm::mat4 GetProjectionMatrix(int viewportWidth, int viewportHeight) const;

#if defined(PLATFORM_WEB)
	// Web still renders through raylib (see the plan) - only that path needs raylib's own camera type.
	::Camera2D GetRaylibCamera() const;
#endif
};
}  // namespace GameResource
}  // namespace Struktur
