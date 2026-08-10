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

struct Color
{
	unsigned char r, g, b, a;
};

struct Rect
{
	float x, y, width, height;
};

constexpr Color ColorWhite{255, 255, 255, 255};
constexpr Color ColorLightGray{200, 200, 200, 255};
constexpr Color ColorGray{130, 130, 130, 255};
constexpr Color ColorDarkGray{80, 80, 80, 255};
constexpr Color ColorYellow{253, 249, 0, 255};
constexpr Color ColorGold{255, 203, 0, 255};
constexpr Color ColorOrange{255, 161, 0, 255};
constexpr Color ColorPink{255, 109, 194, 255};
constexpr Color ColorRed{230, 41, 55, 255};
constexpr Color ColorMaroon{190, 33, 55, 255};
constexpr Color ColorGreen{0, 228, 48, 255};
constexpr Color ColorLime{0, 158, 47, 255};
constexpr Color ColorDarkGreen{0, 117, 44, 255};
constexpr Color ColorSkyBlue{102, 191, 255, 255};
constexpr Color ColorBlue{0, 121, 241, 255};
constexpr Color ColorDarkBlue{0, 82, 172, 255};
constexpr Color ColorPurple{200, 122, 255, 255};
constexpr Color ColorViolet{135, 60, 190, 255};
constexpr Color ColorDarkPurple{112, 31, 126, 255};
constexpr Color ColorBeige{211, 176, 131, 255};
constexpr Color ColorBrown{127, 106, 79, 255};
constexpr Color ColorDarkBrown{76, 63, 47, 255};
constexpr Color ColorBlack{0, 0, 0, 255};
constexpr Color ColorBlank{0, 0, 0, 0};
constexpr Color ColorMagenta{255, 0, 255, 255};
constexpr Color ColorRayWhite{245, 245, 245, 255};
};  // namespace Math
};  // namespace Util
};  // namespace Struktur
