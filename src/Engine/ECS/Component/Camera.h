#pragma once

#include "glm/glm.hpp"

namespace Struktur
{
namespace Component
{
struct Camera
{
	int cameraPriority = 0;

	float zoom  = 1.f;
	float angle = 0.f;

	glm::vec2 offset;
	glm::vec2 deadZone;

	glm::vec2 damping;

	// When set, the view's edges are kept inside bounds (world-space x, y, width, height) - a bounds axis smaller
	// than the view is centred on instead.
	bool confineToBounds = false;
	glm::vec4 bounds{0.f};

	// World point at the centre of the view as of the last CameraSystem update, before shake. Written by
	// CameraSystem only.
	glm::vec2 viewCenter{0.f};

	bool forcePosition   = false;
	float traumaTime     = 1.f;
	float maxOffset      = 0.6f;
	float maxAngle       = 1.f;
	float trauma         = 0.f;
	float shakeAmplitude = 0.1f;
};
}  // namespace Component
}  // namespace Struktur
