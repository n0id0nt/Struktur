#pragma once

#include "box2d/box2d.h"

struct WrenBodyDefinition
{
	b2BodyDef bodyDef;

	WrenBodyDefinition() : bodyDef() {}
	WrenBodyDefinition(const b2BodyDef& b) : bodyDef(b) {}
};

// ===============================================================================
// IMPORTANT NOTE: For all physics shapes the first attribute must be the b2Shape
// ===============================================================================

// Circle shape
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

// Box/Polygon shape
struct WrenPhysicsBoxShape
{
    b2PolygonShape physicsShape;
    WrenPhysicsBoxShape() : physicsShape() {}
    WrenPhysicsBoxShape(const b2PolygonShape& b) : physicsShape(b) {}
    WrenPhysicsBoxShape(float halfWidth, float halfHeight) : physicsShape()
    {
        physicsShape.SetAsBox(halfWidth, halfHeight);
    }
    WrenPhysicsBoxShape(float halfWidth, float halfHeight, float centerX, float centerY, float angle) : physicsShape()
    {
        b2Vec2 center(centerX, centerY);
        physicsShape.SetAsBox(halfWidth, halfHeight, center, angle);
    }
};

// Generic polygon shape
struct WrenPhysicsPolygonShape
{
    b2PolygonShape physicsShape;
    WrenPhysicsPolygonShape() : physicsShape() {}
    WrenPhysicsPolygonShape(const b2PolygonShape& b) : physicsShape(b) {}
};

// Edge shape (for static geometry like walls)
struct WrenPhysicsEdgeShape
{
    b2EdgeShape physicsShape;
    WrenPhysicsEdgeShape() : physicsShape() {}
    WrenPhysicsEdgeShape(const b2EdgeShape& b) : physicsShape(b) {}
    WrenPhysicsEdgeShape(float x1, float y1, float x2, float y2) : physicsShape()
    {
        physicsShape.SetTwoSided(b2Vec2(x1, y1), b2Vec2(x2, y2));
    }
};

// Chain shape (for connecting multiple edges)
struct WrenPhysicsChainShape
{
    b2ChainShape physicsShape;
    WrenPhysicsChainShape() : physicsShape() {}
    WrenPhysicsChainShape(const b2ChainShape& b) : physicsShape(b) {}
};
