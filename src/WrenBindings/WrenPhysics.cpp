#include "WrenPhysics.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "WrenMath.h"

// ============================================================================
// BODY DEFINITION BINDINGS
// ============================================================================

// Allocator
void wren_BodyDefinitionAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "physics", "BodyDefinition", 0);  // Get class into slot 1
	auto* bodyDef = static_cast<WrenBodyDefinition*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition)));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();
}

// Finalizer
void wren_BodyDefinitionFinalize(void* data)
{
	auto* bodyDef = static_cast<WrenBodyDefinition*>(data);
	bodyDef->~WrenBodyDefinition();
}

void wren_BodyDefinitionCreateDynamicBody(WrenVM* vm)
{
	wrenGetVariable(vm, "physics", "BodyDefinition", 0);  // Get class into slot 1
	auto* bodyDef = static_cast<WrenBodyDefinition*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition)));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->type = b2_dynamicBody;
}

void wren_BodyDefinitionCreateStaticBody(WrenVM* vm)
{
	wrenGetVariable(vm, "physics", "BodyDefinition", 0);  // Get class into slot 1
	auto* bodyDef = static_cast<WrenBodyDefinition*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition)));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->type = b2_staticBody;
}

void wren_BodyDefinitionCreateKinematicBody(WrenVM* vm)
{
	wrenGetVariable(vm, "physics", "BodyDefinition", 0);  // Get class into slot 1
	auto* bodyDef = static_cast<WrenBodyDefinition*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition)));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->type = b2_kinematicBody;
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("physics", "BodyDefinition", wren_BodyDefinitionAllocate, wren_BodyDefinitionFinalize, "BodyDefinition class wraps b2BodyDef");

// Register static methods
WREN_CLASS_STATIC("physics", "BodyDefinition", "createDynamicBody()", wren_BodyDefinitionCreateDynamicBody, "Get the BodyDefinition as a dynamic Body");
WREN_CLASS_STATIC("physics", "BodyDefinition", "createStaticBody()", wren_BodyDefinitionCreateStaticBody, "Get the BodyDefinition as a static Body");
WREN_CLASS_STATIC("physics", "BodyDefinition", "createKinematicBody()", wren_BodyDefinitionCreateKinematicBody, "Get the BodyDefinition as a kinematic Body");

// ============================================================================
// CIRCLE SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsCircleShapeAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "physics", "PhysicsCircleShape", 0);  // Get class into slot 1
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsShape)));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 2)
	{
		float radius = (float)wrenGetSlotDouble(vm, 1);
		new (shape) b2CircleShape();
		shape->m_radius = radius;
	}
	else
	{
		new (shape) b2CircleShape();
	}
}

// Finalizer
void wren_PhysicsCircleShapeFinalize(void* data)
{
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(data);
	shape->~WrenPhysicsShape();
}

void wren_PhysicsCircleShapeGetRadius(WrenVM* vm)
{
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(wrenGetSlotForeign(vm, 0));
	b2CircleShape* circleShape = dynamic_cast<b2CircleShape*>(shape);
	if (!circleShape)
	{
		DEBUG_ERROR("PhysicsCircleShape.radius: shape is not a circle");
		return;
	}
	wrenSetSlotDouble(vm, 0, circleShape->m_radius);
}

void wren_PhysicsCircleShapeSetRadius(WrenVM* vm)
{
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(wrenGetSlotForeign(vm, 0));
	b2CircleShape* circleShape = dynamic_cast<b2CircleShape*>(shape);
	if (!circleShape)
	{
		DEBUG_ERROR("PhysicsCircleShape.radius: shape is not a circle");
		return;
	}
	float radius = static_cast<float>(wrenGetSlotDouble(vm, 1));
	circleShape->m_radius = radius;
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("physics", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, wren_PhysicsCircleShapeFinalize, "BodyDefinition class wraps b2BodyDef");

// Register constructors
WREN_CONSTRUCTOR_DOC("physics", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, "Create physics circle shape", );
WREN_CONSTRUCTOR_DOC("physics", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, "Create physics circle shape with a radius components", radius);

// Register methods
WREN_CLASS_METHOD("physics", "PhysicsCircleShape", "radius", wren_PhysicsCircleShapeGetRadius, "Get physics circle shape's radius");
WREN_CLASS_METHOD("physics", "PhysicsCircleShape", "radius=(_)", wren_PhysicsCircleShapeSetRadius, "Set physics circle shape's radius");

// ============================================================================
// SQUARE SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsSquareShapeAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "physics", "PhysicsCircleShape", 0);  // Get class into slot 1
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsShape)));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 5)
	{
		float hx = (float)wrenGetSlotDouble(vm, 1);
		float hy = (float)wrenGetSlotDouble(vm, 2);
		WrenVec2* center = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 3));
		float angle = (float)wrenGetSlotDouble(vm, 4);
		new (shape) b2PolygonShape();
		b2PolygonShape* SquareShape = dynamic_cast<b2PolygonShape*>(shape);
		SquareShape->SetAsBox(hx, hy, b2Vec2(center->value.x, center->value.y), angle);
	}
	else
	{
		float hx = (float)wrenGetSlotDouble(vm, 1);
		float hy = (float)wrenGetSlotDouble(vm, 2);
		new (shape) b2PolygonShape();
		b2PolygonShape* SquareShape = dynamic_cast<b2PolygonShape*>(shape);
		SquareShape->SetAsBox(hx, hy);
	}
}

// Finalizer
void wren_PhysicsSquareShapeFinalize(void* data)
{
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(data);
	shape->~WrenPhysicsShape();
}

void wren_PhysicsSquareShapeGetRadius(WrenVM* vm)
{
	WrenPhysicsShape* shape = static_cast<WrenPhysicsShape*>(wrenGetSlotForeign(vm, 0));
	b2PolygonShape* SquareShape = dynamic_cast<b2PolygonShape*>(shape);
	if (!SquareShape)
	{
		DEBUG_ERROR("PhysicsSquareShape.radius: shape is not a Square");
		return;
	}
	wrenSetSlotDouble(vm, 0, SquareShape->m_radius);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("physics", "PhysicsSquareShape", wren_PhysicsSquareShapeAllocate, wren_PhysicsSquareShapeFinalize, "BodyDefinition class wraps b2BodyDef");

// Register constructors
WREN_CONSTRUCTOR_DOC("physics", "PhysicsSquareShape", wren_PhysicsSquareShapeAllocate, "Create physics Square shape with width, height, center, and angle", hx, hy, center, angle);
WREN_CONSTRUCTOR_DOC("physics", "PhysicsSquareShape", wren_PhysicsSquareShapeAllocate, "Create physics Square shape with width and height", hx, hy);

// Register methods
WREN_CLASS_METHOD("physics", "PhysicsSquareShape", "radius", wren_PhysicsSquareShapeGetRadius, "Get physics Square shape's radius");
