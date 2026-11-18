#pragma once

#include "Engine/Util/MathUtil.h"
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/quaternion.hpp>

struct WrenVec2
{
	glm::vec2 value;

	WrenVec2()
	    : value(0.0f, 0.0f)
	{
	}
	WrenVec2(float x, float y)
	    : value(x, y)
	{
	}
	WrenVec2(const glm::vec2& vector)
	    : value(vector)
	{
	}
};

struct WrenVec3
{
	glm::vec3 value;

	WrenVec3()
	    : value(0.0f, 0.0f, 0.0f)
	{
	}
	WrenVec3(float x, float y, float z)
	    : value(x, y, z)
	{
	}
	WrenVec3(const glm::vec3& vector)
	    : value(vector)
	{
	}
};

struct WrenVec4
{
	glm::vec4 value;

	WrenVec4()
	    : value(0.0f, 0.0f, 0.0f, 0.0f)
	{
	}
	WrenVec4(float x, float y, float z, float w)
	    : value(x, y, z, w)
	{
	}
	WrenVec4(const glm::vec4& vector)
	    : value(vector)
	{
	}
};

struct WrenQuat
{
	glm::quat value;

	WrenQuat()
	    : value(1.0f, 0.0f, 0.0f, 0.0f)
	{
	}  // Identity quaternion
	WrenQuat(float w, float x, float y, float z)
	    : value(w, x, y, z)
	{
	}
	WrenQuat(const glm::quat& quaternion)
	    : value(quaternion)
	{
	}
};

struct WrenMat4
{
	glm::mat4 value;

	WrenMat4()
	    : value(1.0f)
	{
	}  // Identity Matrix
	WrenMat4(const glm::mat4& matrix)
	    : value(matrix)
	{
	}
};

struct WrenRect
{
	Struktur::Util::Math::Rect value;

	WrenRect(const Struktur::Util::Math::Rect& rect)
	    : value(rect)
	{
	}
};
