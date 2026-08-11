#include "Camera.h"

#include <cmath>

#include "glm/gtc/matrix_transform.hpp"

glm::vec2 Struktur::World::Camera::WorldPosToScreenPos(const glm::vec2& worldPos)
{
	glm::vec2 p   = worldPos - target;
	float rad     = glm::radians(rotation);
	float cs      = std::cos(rad);
	float sn      = std::sin(rad);
	glm::vec2 rot = glm::vec2{p.x * cs - p.y * sn, p.x * sn + p.y * cs};
	return rot * zoom + offset;
}

glm::vec2 Struktur::World::Camera::ScreenPosToWorldPos(const glm::vec2& screenPos)
{
	glm::vec2 p   = (screenPos - offset) / zoom;
	float rad     = -glm::radians(rotation);
	float cs      = std::cos(rad);
	float sn      = std::sin(rad);
	glm::vec2 rot = glm::vec2{p.x * cs - p.y * sn, p.x * sn + p.y * cs};
	return rot + target;
}

glm::mat4 Struktur::World::Camera::GetViewMatrix() const
{
	glm::mat4 view(1.0f);
	view = glm::translate(view, glm::vec3(offset, 0.0f));
	view = glm::rotate(view, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
	view = glm::translate(view, glm::vec3(-target, 0.0f));
	return view;
}

glm::mat4 Struktur::World::Camera::GetProjectionMatrix(int viewportWidth, int viewportHeight) const
{
	// Depth isn't meaningful for 2D sprites (draw order comes from submission order, not the depth buffer),
	// so the exact near/far range and OpenGL-style NDC convention glm::ortho assumes don't matter here.
	return glm::ortho(0.0f, (float)viewportWidth, (float)viewportHeight, 0.0f, -1.0f, 1.0f);
}
