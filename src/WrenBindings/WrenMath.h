#pragma once

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <raylib.h>

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
	WrenVec2(const glm::vec2& v)
	    : value(v)
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
	WrenVec3(const glm::vec3& v)
	    : value(v)
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
	WrenVec4(const glm::vec4& v)
	    : value(v)
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
	WrenQuat(const glm::quat& q)
	    : value(q)
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
	WrenMat4(const glm::mat4& m)
	    : value(m)
	{
	}
};

struct WrenRect
{
	::Rectangle value;

	WrenRect(const ::Rectangle& m)
	    : value(m)
	{
	}
};
