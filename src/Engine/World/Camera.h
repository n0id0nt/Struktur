#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Struktur::World
{
// The 3D camera the world is drawn through. It lives on the GameContext; System::CameraSystem moves it. Positions are
// in the world's coordinates (x / z absolute metres, y relative to the road height under the rider) and kept in double
// precision; the matrices are built relative to the camera's x / z, so rendering stays precise over any distance.
struct Camera
{
	glm::dvec3 position{0.0, 2.2, 0.0};
	glm::dvec3 target{10.0, 2.2, 0.0};
	double fieldOfViewDegrees = 62.0;  // vertical
	float nearPlane           = 0.3f;
	float farPlane            = 1200.0f;

	// The eye and target in the frame the world is rendered in: relative to (position.x, 0, position.z).
	glm::vec3 GetEye() const
	{
		return glm::vec3(0.0f, static_cast<float>(position.y), 0.0f);
	}
	glm::vec3 GetRelativeTarget() const
	{
		return glm::vec3(static_cast<float>(target.x - position.x), static_cast<float>(target.y),
		                 static_cast<float>(target.z - position.z));
	}
	glm::mat4 GetViewMatrix() const
	{
		return glm::lookAtRH(GetEye(), GetRelativeTarget(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	// `homogeneousDepth`: clip-space depth is -1..1 (bgfx::getCaps()->homogeneousDepth) rather than 0..1.
	glm::mat4 GetProjectionMatrix(float aspect, bool homogeneousDepth) const
	{
		const float fieldOfView = glm::radians(static_cast<float>(fieldOfViewDegrees));
		return homogeneousDepth ? glm::perspectiveRH_NO(fieldOfView, aspect, nearPlane, farPlane)
		                        : glm::perspectiveRH_ZO(fieldOfView, aspect, nearPlane, farPlane);
	}
};
}  // namespace Struktur::World
