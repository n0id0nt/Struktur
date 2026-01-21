#include "WrenPhysics.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// BODY TYPES
// ============================================================================

WREN_ENUM("physics", BodyType, "Enum of the different phyics body types",
	WREN_ENUM_PAIR("STATIC_BODY", b2_staticBody),
	WREN_ENUM_PAIR("KINEMATIC_BODY", b2_kinematicBody),
	WREN_ENUM_PAIR("DYNAMIC_BODY", b2_dynamicBody),
	);

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
	double type = static_cast<double>(bodyDef->bodyDef.type);
	wrenSetSlotDouble(vm, 0, type);
}

// BodyDefinition.type=(_)
void wren_BodyDefinitionSetType(WrenVM* vm)
{
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 0));
	b2BodyType bodyType = static_cast<b2BodyType>(wrenGetSlotDouble(vm, 1));
	bodyDef->bodyDef.type = bodyType;
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("physics", "BodyDefinition", wren_BodyDefinitionAllocate, wren_BodyDefinitionFinalize, "BodyDefinition class wraps b2BodyDef");

// Register constructors
WREN_CONSTRUCTOR("physics", "BodyDefinition", "new(_)", wren_BodyDefinitionNew, "Create the physics body definition, takes the body type as the argement");

// Register static methods
WREN_CLASS_METHOD("physics", "BodyDefinition", "type", wren_BodyDefinitionGetType, "Gets the Body Definition Type");
WREN_CLASS_METHOD("physics", "BodyDefinition", "type=(_)", wren_BodyDefinitionSetType, "Sets the Body Definition Type");

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
	WrenPhysicsCircleShape* bodyDef = (WrenPhysicsCircleShape*)data;
	bodyDef->~WrenPhysicsCircleShape();
}

// PhysicsCircleShape.new(_)
void wren_PhysicsCircleNew(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
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
	physicsShape->physicsShape.m_radius = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("physics", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, wren_PhysicsCircleShapeFinalize, "BodyDefinition class wraps b2BodyDef");

// Register constructors
WREN_CONSTRUCTOR("physics", "PhysicsCircleShape", "new()", wren_PhysicsCircleNew, "Create physics circle shape");
WREN_CONSTRUCTOR("physics", "PhysicsCircleShape", "new(_)", wren_PhysicsCircleNew, "Create physics circle shape with a radius components");

// Register methods
WREN_CLASS_METHOD("physics", "PhysicsCircleShape", "radius", wren_PhysicsCircleShapeGetRadius, "Get physics circle shape's radius");
WREN_CLASS_METHOD("physics", "PhysicsCircleShape", "radius=(_)", wren_PhysicsCircleShapeSetRadius, "Set physics circle shape's radius");
