#pragma once

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

	glm::vec2 WorldPosToScreenPos(const glm::vec2& worldPos);
	glm::vec2 ScreenPosToWorldPos(const glm::vec2& screenPos);

	// Standard 2D camera view matrix: translate by -target, rotate, scale by zoom, translate by +offset.
	glm::mat4 GetViewMatrix() const;
	// Screen-space orthographic projection (origin top-left, Y down), matching raylib's prior screen convention.
	glm::mat4 GetProjectionMatrix(int viewportWidth, int viewportHeight) const;
};
}  // namespace GameResource
}  // namespace Struktur
