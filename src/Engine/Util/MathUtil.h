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
};  // namespace Math
};  // namespace Util
};  // namespace Struktur
