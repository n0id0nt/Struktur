#pragma once

#include "box2d/box2d.h"

struct WrenBodyDefinition
{
	b2BodyDef bodyDef;

	WrenBodyDefinition() : bodyDef() {}
	WrenBodyDefinition(const b2BodyDef& b) : bodyDef(b) {}
};

struct WrenPhysicsCircleShape
{
	b2CircleShape physicsShape;

	WrenPhysicsCircleShape() : physicsShape() {}
	WrenPhysicsCircleShape(const b2CircleShape& b) : physicsShape(b) {}
	WrenPhysicsCircleShape(float radius) : physicsShape()
	{
		physicsShape.m_radius = radius;
	}
};
