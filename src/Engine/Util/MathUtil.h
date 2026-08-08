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

struct Color
{
	unsigned char r, g, b, a;
};

struct Rect
{
	float x, y, width, height;
};

constexpr Color ColorWhite{255, 255, 255, 255};
};  // namespace Math
};  // namespace Util
};  // namespace Struktur
