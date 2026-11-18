#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
namespace Util
{
namespace Math
{
inline float AngleZFromQuat(const glm::quat& rotation)
{
	return std::atan2(2.0f * (rotation.w * rotation.z + rotation.x * rotation.y),
	                  1.0f - 2.0f * (rotation.y * rotation.y + rotation.z * rotation.z));
}

// Replaces raylib's raymath.h ::Lerp - same signature/behaviour (unclamped linear interpolation).
inline float Lerp(float start, float end, float amount)
{
	return start + amount * (end - start);
}

// Maps a float to a uint32 that sorts (as an unsigned integer) in the same order as the float,
// including negatives - lets values like a negative world Y participate in a packed integer sort key correctly.
inline uint32_t FloatToSortableBits(float value)
{
	uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	uint32_t mask = (bits & 0x80000000u) ? 0xFFFFFFFFu : 0x80000000u;
	return bits ^ mask;
}

struct Rect
{
	float x, y, width, height;
};

inline bool RectOverlaps(const Rect& rect, float minX, float minY, float maxX, float maxY)
{
	return rect.x < maxX && rect.x + rect.width > minX && rect.y < maxY && rect.y + rect.height > minY;
}
};  // namespace Math
};  // namespace Util
};  // namespace Struktur
