#pragma once

#include <cmath>

#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
namespace Util
{
namespace Math
{
inline float AngleZFromQuat(const glm::quat& q)
{
	return std::atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
}

// Replaces raylib's raymath.h ::Lerp - same signature/behaviour (unclamped linear interpolation).
inline float Lerp(float start, float end, float amount)
{
	return start + amount * (end - start);
}

struct Rect
{
	float x, y, width, height;
};
};  // namespace Math
};  // namespace Util
};  // namespace Struktur
