#include "WrenPhysics.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"


// ============================================================================
// BODY DEFINITION BINDINGS
// ============================================================================

// Allocator
void wren_BodyDefinitionAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();
}

// Finalizer
void wren_BodyDefinitionFinalize(void* data)
{
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)data;
	bodyDef->~WrenBodyDefinition();
}

void wren_BodyDefinitionCreateDynamicBody(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = b2_dynamicBody;
}

void wren_BodyDefinitionCreateStaticBody(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = b2_staticBody;
}

void wren_BodyDefinitionCreateKinematicBody(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = b2_kinematicBody;
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "BodyDefinition", wren_BodyDefinitionAllocate, wren_BodyDefinitionFinalize, "BodyDefinition class wraps b2BodyDef");

// Register static methods
WREN_CLASS_STATIC("game", "BodyDefinition", "createDynamicBody()", wren_BodyDefinitionCreateDynamicBody, "Get the BodyDefinition as a dynamic Body");
WREN_CLASS_STATIC("game", "BodyDefinition", "createStaticBody()", wren_BodyDefinitionCreateStaticBody, "Get the BodyDefinition as a static Body");
WREN_CLASS_STATIC("game", "BodyDefinition", "createKinematicBody()", wren_BodyDefinitionCreateKinematicBody, "Get the BodyDefinition as a kinematic Body");

// ============================================================================
// CIRCLE SHAPE BINDINGS
// ============================================================================

// Allocator
void wren_PhysicsCircleShapeAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "PhysicsCircleShape", 0);  // Get class into slot 1
	WrenPhysicsCircleShape* bodyDef = (WrenPhysicsCircleShape*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsCircleShape));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 2)
	{
		float radius = (float)wrenGetSlotDouble(vm, 1);
		new (bodyDef) WrenPhysicsCircleShape(radius);
	}
	else
	{
		new (bodyDef) WrenPhysicsCircleShape();
	}
}

// Finalizer
void wren_PhysicsCircleShapeFinalize(void* data)
{
	WrenPhysicsCircleShape* bodyDef = (WrenPhysicsCircleShape*)data;
	bodyDef->~WrenPhysicsCircleShape();
}


void wren_PhysicsCircleShapeGetRadius(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = (WrenPhysicsCircleShape*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, physicsShape->physicsShape.m_radius);
}

void wren_PhysicsCircleShapeSetRadius(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = (WrenPhysicsCircleShape*)wrenGetSlotForeign(vm, 0);
	physicsShape->physicsShape.m_radius = (float)wrenGetSlotDouble(vm, 1);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, wren_PhysicsCircleShapeFinalize, "BodyDefinition class wraps b2BodyDef");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, "Create physics circle shape", );
WREN_CONSTRUCTOR_DOC("game", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, "Create physics circle shape with a radius components", radius);

// Register methods
WREN_CLASS_METHOD("game", "PhysicsCircleShape", "radius", wren_PhysicsCircleShapeGetRadius, "Get physics circle shape's radius");
WREN_CLASS_METHOD("game", "PhysicsCircleShape", "radius=(_)", wren_PhysicsCircleShapeSetRadius, "Set physics circle shape's radius");
