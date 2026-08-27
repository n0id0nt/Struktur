#include "WrenPhysics.h"

#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "WrenMath.h"
#include "wren.hpp"

// ============================================================================
// BODY DEFINITION BINDINGS
// ============================================================================

// Allocator
void wren_BodyDefinitionAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));
}

// Finalizer
void wren_BodyDefinitionFinalize(void* data)
{
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)data;
	bodyDef->~WrenBodyDefinition();
}

// BodyDefinition.new(_)
void wren_BodyDefinitionNew(WrenVM* vm)
{
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 0));

	b2BodyType bodyType = static_cast<b2BodyType>(wrenGetSlotDouble(vm, 1));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = bodyType;
}

// BodyDefinition.type
void wren_BodyDefinitionGetType(WrenVM* vm)
{
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 0));
	double type                 = static_cast<double>(bodyDef->bodyDef.type);
	wrenSetSlotDouble(vm, 0, type);
}

// BodyDefinition.type=(_)
void wren_BodyDefinitionSetType(WrenVM* vm)
{
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 0));
	b2BodyType bodyType         = static_cast<b2BodyType>(wrenGetSlotDouble(vm, 1));
	bodyDef->bodyDef.type       = bodyType;
}

// ============================================================================
// CIRCLE SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsCircleShapeAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsCircleShape));
}

// Finalizer
void wren_PhysicsCircleShapeFinalize(void* data)
{
	WrenPhysicsCircleShape* shape = (WrenPhysicsCircleShape*)data;
	shape->~WrenPhysicsCircleShape();
}

// PhysicsCircleShape.new(_)
void wren_PhysicsCircleNew(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 0));

	if (wrenGetSlotCount(vm) >= 2)
	{
		float radius = static_cast<float>(wrenGetSlotDouble(vm, 1));
		new (physicsShape) WrenPhysicsCircleShape(radius);
	}
	else
	{
		new (physicsShape) WrenPhysicsCircleShape();
	}
}

void wren_PhysicsCircleShapeGetRadius(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, physicsShape->physicsShape.m_radius);
}

void wren_PhysicsCircleShapeSetRadius(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 0));
	physicsShape->physicsShape.m_radius  = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

// ============================================================================
// BOX SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsBoxShapeAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsBoxShape));
}

// Finalizer
void wren_PhysicsBoxShapeFinalize(void* data)
{
	WrenPhysicsBoxShape* shape = (WrenPhysicsBoxShape*)data;
	shape->~WrenPhysicsBoxShape();
}

// PhysicsBoxShape.new(_,_) or new(_,_,_,_,_)
void wren_PhysicsBoxNew(WrenVM* vm)
{
	WrenPhysicsBoxShape* physicsShape = static_cast<WrenPhysicsBoxShape*>(wrenGetSlotForeign(vm, 0));

	if (wrenGetSlotCount(vm) >= 6)
	{
		// new(halfWidth, halfHeight, centerX, centerY, angle)
		float halfWidth  = static_cast<float>(wrenGetSlotDouble(vm, 1));
		float halfHeight = static_cast<float>(wrenGetSlotDouble(vm, 2));
		float centerX    = static_cast<float>(wrenGetSlotDouble(vm, 3));
		float centerY    = static_cast<float>(wrenGetSlotDouble(vm, 4));
		float angle      = static_cast<float>(wrenGetSlotDouble(vm, 5));
		new (physicsShape) WrenPhysicsBoxShape(halfWidth, halfHeight, centerX, centerY, angle);
	}
	else if (wrenGetSlotCount(vm) >= 3)
	{
		// new(halfWidth, halfHeight)
		float halfWidth  = static_cast<float>(wrenGetSlotDouble(vm, 1));
		float halfHeight = static_cast<float>(wrenGetSlotDouble(vm, 2));
		new (physicsShape) WrenPhysicsBoxShape(halfWidth, halfHeight);
	}
	else
	{
		new (physicsShape) WrenPhysicsBoxShape();
	}
}

void wren_PhysicsBoxShapeSetAsBox(WrenVM* vm)
{
	WrenPhysicsBoxShape* physicsShape = static_cast<WrenPhysicsBoxShape*>(wrenGetSlotForeign(vm, 0));

	if (wrenGetSlotCount(vm) >= 6)
	{
		float halfWidth  = static_cast<float>(wrenGetSlotDouble(vm, 1));
		float halfHeight = static_cast<float>(wrenGetSlotDouble(vm, 2));
		float centerX    = static_cast<float>(wrenGetSlotDouble(vm, 3));
		float centerY    = static_cast<float>(wrenGetSlotDouble(vm, 4));
		float angle      = static_cast<float>(wrenGetSlotDouble(vm, 5));
		b2Vec2 center(centerX, centerY);
		physicsShape->physicsShape.SetAsBox(halfWidth, halfHeight, center, angle);
	}
	else if (wrenGetSlotCount(vm) >= 3)
	{
		float halfWidth  = static_cast<float>(wrenGetSlotDouble(vm, 1));
		float halfHeight = static_cast<float>(wrenGetSlotDouble(vm, 2));
		physicsShape->physicsShape.SetAsBox(halfWidth, halfHeight);
	}
}

// ============================================================================
// POLYGON SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsPolygonShapeAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsPolygonShape));
}

// Finalizer
void wren_PhysicsPolygonShapeFinalize(void* data)
{
	WrenPhysicsPolygonShape* shape = (WrenPhysicsPolygonShape*)data;
	shape->~WrenPhysicsPolygonShape();
}

// PhysicsPolygonShape.new()
void wren_PhysicsPolygonNew(WrenVM* vm)
{
	WrenPhysicsPolygonShape* physicsShape = static_cast<WrenPhysicsPolygonShape*>(wrenGetSlotForeign(vm, 0));
	new (physicsShape) WrenPhysicsPolygonShape();
}

// setVertices([list of Vec2])
void wren_PhysicsPolygonShapeSetVertices(WrenVM* vm)
{
	WrenPhysicsPolygonShape* physicsShape = static_cast<WrenPhysicsPolygonShape*>(wrenGetSlotForeign(vm, 0));

	// Expect a list in slot 1
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		wrenSetSlotString(vm, 0, "Expected a list of Vec2");
		wrenAbortFiber(vm, 0);
		return;
	}

	int count = wrenGetListCount(vm, 1);
	if (count < 3 || count > b2_maxPolygonVertices)
	{
		wrenSetSlotString(vm, 0, "Polygon must have 3-8 vertices");
		wrenAbortFiber(vm, 0);
		return;
	}

	std::vector<b2Vec2> vertices;
	vertices.reserve(count);

	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
		vertices.push_back(b2Vec2(vec->value.x, vec->value.y));
	}

	physicsShape->physicsShape.Set(vertices.data(), static_cast<int32>(vertices.size()));
}

// ============================================================================
// EDGE SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsEdgeShapeAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsEdgeShape));
}

// Finalizer
void wren_PhysicsEdgeShapeFinalize(void* data)
{
	WrenPhysicsEdgeShape* shape = (WrenPhysicsEdgeShape*)data;
	shape->~WrenPhysicsEdgeShape();
}

// PhysicsEdgeShape.new(_,_,_,_)
void wren_PhysicsEdgeNew(WrenVM* vm)
{
	WrenPhysicsEdgeShape* physicsShape = static_cast<WrenPhysicsEdgeShape*>(wrenGetSlotForeign(vm, 0));

	if (wrenGetSlotCount(vm) >= 5)
	{
		float x1 = static_cast<float>(wrenGetSlotDouble(vm, 1));
		float y1 = static_cast<float>(wrenGetSlotDouble(vm, 2));
		float x2 = static_cast<float>(wrenGetSlotDouble(vm, 3));
		float y2 = static_cast<float>(wrenGetSlotDouble(vm, 4));
		new (physicsShape) WrenPhysicsEdgeShape(x1, y1, x2, y2);
	}
	else
	{
		new (physicsShape) WrenPhysicsEdgeShape();
	}
}

void wren_PhysicsEdgeShapeSetTwoSided(WrenVM* vm)
{
	WrenPhysicsEdgeShape* physicsShape = static_cast<WrenPhysicsEdgeShape*>(wrenGetSlotForeign(vm, 0));
	float x1                           = static_cast<float>(wrenGetSlotDouble(vm, 1));
	float y1                           = static_cast<float>(wrenGetSlotDouble(vm, 2));
	float x2                           = static_cast<float>(wrenGetSlotDouble(vm, 3));
	float y2                           = static_cast<float>(wrenGetSlotDouble(vm, 4));
	physicsShape->physicsShape.SetTwoSided(b2Vec2(x1, y1), b2Vec2(x2, y2));
}

void wren_PhysicsEdgeShapeSetOneSided(WrenVM* vm)
{
	WrenPhysicsEdgeShape* physicsShape = static_cast<WrenPhysicsEdgeShape*>(wrenGetSlotForeign(vm, 0));

	// ghost0, v1, v2, ghost3
	float g0x = static_cast<float>(wrenGetSlotDouble(vm, 1));
	float g0y = static_cast<float>(wrenGetSlotDouble(vm, 2));
	float v1x = static_cast<float>(wrenGetSlotDouble(vm, 3));
	float v1y = static_cast<float>(wrenGetSlotDouble(vm, 4));
	float v2x = static_cast<float>(wrenGetSlotDouble(vm, 5));
	float v2y = static_cast<float>(wrenGetSlotDouble(vm, 6));
	float g3x = static_cast<float>(wrenGetSlotDouble(vm, 7));
	float g3y = static_cast<float>(wrenGetSlotDouble(vm, 8));

	physicsShape->physicsShape.SetOneSided(b2Vec2(g0x, g0y), b2Vec2(v1x, v1y), b2Vec2(v2x, v2y), b2Vec2(g3x, g3y));
}

// ============================================================================
// CHAIN SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsChainShapeAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsChainShape));
}

// Finalizer
void wren_PhysicsChainShapeFinalize(void* data)
{
	WrenPhysicsChainShape* shape = (WrenPhysicsChainShape*)data;
	shape->~WrenPhysicsChainShape();
}

// PhysicsChainShape.new()
void wren_PhysicsChainNew(WrenVM* vm)
{
	WrenPhysicsChainShape* physicsShape = static_cast<WrenPhysicsChainShape*>(wrenGetSlotForeign(vm, 0));
	new (physicsShape) WrenPhysicsChainShape();
}

// createLoop([list of Vec2])
void wren_PhysicsChainShapeCreateLoop(WrenVM* vm)
{
	WrenPhysicsChainShape* physicsShape = static_cast<WrenPhysicsChainShape*>(wrenGetSlotForeign(vm, 0));

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		wrenSetSlotString(vm, 0, "Expected a list of Vec2");
		wrenAbortFiber(vm, 0);
		return;
	}

	int count = wrenGetListCount(vm, 1);
	if (count < 3)
	{
		wrenSetSlotString(vm, 0, "Chain loop must have at least 3 vertices");
		wrenAbortFiber(vm, 0);
		return;
	}

	std::vector<b2Vec2> vertices;
	vertices.reserve(count);

	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
		vertices.push_back(b2Vec2(vec->value.x, vec->value.y));
	}

	physicsShape->physicsShape.CreateLoop(vertices.data(), static_cast<int32>(vertices.size()));
}

// createChain([list of Vec2])
void wren_PhysicsChainShapeCreateChain(WrenVM* vm)
{
	WrenPhysicsChainShape* physicsShape = static_cast<WrenPhysicsChainShape*>(wrenGetSlotForeign(vm, 0));

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		wrenSetSlotString(vm, 0, "Expected a list of Vec2");
		wrenAbortFiber(vm, 0);
		return;
	}

	int count = wrenGetListCount(vm, 1);
	if (count < 2)
	{
		wrenSetSlotString(vm, 0, "Chain must have at least 2 vertices");
		wrenAbortFiber(vm, 0);
		return;
	}

	std::vector<b2Vec2> vertices;
	vertices.reserve(count);

	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
		vertices.push_back(b2Vec2(vec->value.x, vec->value.y));
	}

	physicsShape->physicsShape.CreateChain(vertices.data(), static_cast<int32>(vertices.size()), b2Vec2_zero,
	                                       b2Vec2_zero);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(Physics)
{
	WREN_ENUM(registry, "physics", BodyType, "Enum of the different phyics body types",
	          WREN_ENUM_PAIR("STATIC_BODY", b2_staticBody), WREN_ENUM_PAIR("KINEMATIC_BODY", b2_kinematicBody),
	          WREN_ENUM_PAIR("DYNAMIC_BODY", b2_dynamicBody), );

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "physics", "BodyDefinition", wren_BodyDefinitionAllocate, wren_BodyDefinitionFinalize,
	                   "BodyDefinition class wraps b2BodyDef");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "physics", "BodyDefinition", "new(_)", wren_BodyDefinitionNew,
	                 "Create the physics body definition, takes the body type as the argement");

	// Register static methods
	WREN_CLASS_METHOD(registry, "physics", "BodyDefinition", "type", wren_BodyDefinitionGetType,
	                  "Gets the Body Definition Type");
	WREN_CLASS_METHOD(registry, "physics", "BodyDefinition", "type=(_)", wren_BodyDefinitionSetType,
	                  "Sets the Body Definition Type");

	// Register PhysicsCircleShape foreign class
	WREN_FOREIGN_CLASS(registry, "physics", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate,
	                   wren_PhysicsCircleShapeFinalize, "PhysicsCircleShape class wraps b2CircleShape");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsCircleShape", "new()", wren_PhysicsCircleNew,
	                 "Create physics circle shape");
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsCircleShape", "new(_)", wren_PhysicsCircleNew,
	                 "Create physics circle shape with radius");

	// Register methods
	WREN_CLASS_METHOD(registry, "physics", "PhysicsCircleShape", "radius", wren_PhysicsCircleShapeGetRadius,
	                  "Get physics circle shape's radius");
	WREN_CLASS_METHOD(registry, "physics", "PhysicsCircleShape", "radius=(_)", wren_PhysicsCircleShapeSetRadius,
	                  "Set physics circle shape's radius");

	// Register PhysicsBoxShape foreign class
	WREN_FOREIGN_CLASS(registry, "physics", "PhysicsBoxShape", wren_PhysicsBoxShapeAllocate,
	                   wren_PhysicsBoxShapeFinalize, "PhysicsBoxShape class wraps b2PolygonShape as box");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsBoxShape", "new()", wren_PhysicsBoxNew, "Create physics box shape");
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsBoxShape", "new(_,_)", wren_PhysicsBoxNew,
	                 "Create physics box shape with half-width and half-height");
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsBoxShape", "new(_,_,_,_,_)", wren_PhysicsBoxNew,
	                 "Create physics box shape with half-width, half-height, center, and angle");

	// Register methods
	WREN_CLASS_METHOD(registry, "physics", "PhysicsBoxShape", "setAsBox(_,_)", wren_PhysicsBoxShapeSetAsBox,
	                  "Set as box with half-width and half-height");
	WREN_CLASS_METHOD(registry, "physics", "PhysicsBoxShape", "setAsBox(_,_,_,_,_)", wren_PhysicsBoxShapeSetAsBox,
	                  "Set as box with half-width, half-height, center, and angle");

	// Register PhysicsPolygonShape foreign class
	WREN_FOREIGN_CLASS(registry, "physics", "PhysicsPolygonShape", wren_PhysicsPolygonShapeAllocate,
	                   wren_PhysicsPolygonShapeFinalize, "PhysicsPolygonShape class wraps b2PolygonShape");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsPolygonShape", "new()", wren_PhysicsPolygonNew,
	                 "Create physics polygon shape");

	// Register methods
	WREN_CLASS_METHOD(registry, "physics", "PhysicsPolygonShape", "setVertices(_)", wren_PhysicsPolygonShapeSetVertices,
	                  "Set polygon vertices from list of Vec2");

	// Register PhysicsEdgeShape foreign class
	WREN_FOREIGN_CLASS(registry, "physics", "PhysicsEdgeShape", wren_PhysicsEdgeShapeAllocate,
	                   wren_PhysicsEdgeShapeFinalize, "PhysicsEdgeShape class wraps b2EdgeShape");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsEdgeShape", "new()", wren_PhysicsEdgeNew,
	                 "Create physics edge shape");
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsEdgeShape", "new(_,_,_,_)", wren_PhysicsEdgeNew,
	                 "Create physics edge shape with two points");

	// Register methods
	WREN_CLASS_METHOD(registry, "physics", "PhysicsEdgeShape", "setTwoSided(_,_,_,_)", wren_PhysicsEdgeShapeSetTwoSided,
	                  "Set two-sided edge with two points");
	WREN_CLASS_METHOD(registry, "physics", "PhysicsEdgeShape", "setOneSided(_,_,_,_,_,_,_,_)",
	                  wren_PhysicsEdgeShapeSetOneSided, "Set one-sided edge with ghost vertices");

	// Register PhysicsChainShape foreign class
	WREN_FOREIGN_CLASS(registry, "physics", "PhysicsChainShape", wren_PhysicsChainShapeAllocate,
	                   wren_PhysicsChainShapeFinalize, "PhysicsChainShape class wraps b2ChainShape");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "physics", "PhysicsChainShape", "new()", wren_PhysicsChainNew,
	                 "Create physics chain shape");

	// Register methods
	WREN_CLASS_METHOD(registry, "physics", "PhysicsChainShape", "createLoop(_)", wren_PhysicsChainShapeCreateLoop,
	                  "Create closed loop from list of Vec2");
	WREN_CLASS_METHOD(registry, "physics", "PhysicsChainShape", "createChain(_)", wren_PhysicsChainShapeCreateChain,
	                  "Create open chain from list of Vec2");
}
