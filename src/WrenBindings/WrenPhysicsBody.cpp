// The PhysicsBody script component's bindings: part of the physics library, registered into gameObjectComponents.
#include "Debug/Assertions.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "WrenGameObjectComponents.h"
#include "WrenMath.h"
#include "WrenPhysics.h"
#include "entt/entt.hpp"
#include "wren.hpp"

// ============================================================================
// PHYSICS BODY BINDINGS
// ============================================================================

// PhysicsBody.create(entity, bodyDef, shape) -> PhysicsBody
void wren_PhysicsBodyCreate(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry                       = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem                            = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	double entityId             = wrenGetSlotDouble(vm, 1);
	entt::entity entity         = static_cast<entt::entity>(entityId);
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 2));
	// This can possibly cause issues if the underlying bs shape does not have a b2shape as its fist attribute
	b2Shape* shape = static_cast<b2Shape*>(wrenGetSlotForeign(vm, 3));

	Struktur::Component::PhysicsBody& physicsBodyComponent =
	    physicsSystem.CreatePhysicsBody(*context, entity, bodyDef->bodyDef, *shape);

	wrenGetVariable(vm, "gameObjectComponents", "PhysicsBody", 1);  // Get class into slot 1
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenPhysicsBody));

	// PhysicsBody.new() - identity
	new (physicsBody) WrenPhysicsBody(entity, &physicsBodyComponent);
}

void wren_PhysicsBodySetFixedRotation(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	bool fixedRotation           = (float)wrenGetSlotBool(vm, 1);
	physicsBody->component->body->SetFixedRotation(fixedRotation);
}

// PhysicsBody.setLinearVelocity(entity, velocity)
void wren_PhysicsBodyStaticSetLinearVelocity(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                                 = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenVec2* velocity  = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

	auto* physicsBodyComponent = registry.try_get<Struktur::Component::PhysicsBody>(entity);

	if (!physicsBodyComponent)
	{
		return;
	}

	b2Vec2 b2Velecity = b2Vec2(velocity->value.x, velocity->value.y);
	physicsBodyComponent->body->SetLinearVelocity(b2Velecity);
}

// PhysicsBody.linearVelocity = velocity
void wren_PhysicsBodySetLinearVelocity(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);

	WrenVec2* velocity = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));

	b2Vec2 b2Velecity = b2Vec2(velocity->value.x, velocity->value.y);
	physicsBody->component->body->SetLinearVelocity(b2Velecity);
}

// PhysicsBody.setCollisionFilter(categoryBits, maskBits) - see physics.CollisionLayers for allocating the bits
void wren_PhysicsBodySetCollisionFilter(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem                            = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	uint16_t categoryBits        = static_cast<uint16_t>(wrenGetSlotDouble(vm, 1));
	uint16_t maskBits            = static_cast<uint16_t>(wrenGetSlotDouble(vm, 2));

	physicsSystem.SetCollisionFilter(*physicsBody->component, categoryBits, maskBits);
}

void wren_PhysicsBodyGetIsSensor(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem                            = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, physicsSystem.IsSensor(*physicsBody->component));
}

void wren_PhysicsBodySetIsSensor(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem                            = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	physicsSystem.SetSensor(*physicsBody->component, wrenGetSlotBool(vm, 1));
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(PhysicsBody)
{
	// Register PhysicsBody foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "PhysicsBody", wren_PhysicsBodyAllocate,
	                   wren_PhysicsBodyFinalize, "PhysicsBody class wraps PhysicsBody component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "fixedRotation=(_)",
	                  wren_PhysicsBodySetFixedRotation, "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "linearVelocity",
	                  wren_PhysicsBodySetLinearVelocity, "Sets the linear velocity of a physics body.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "setCollisionFilter(_,_)",
	                  wren_PhysicsBodySetCollisionFilter,
	                  "Sets which collision layer this body belongs to (categoryBits) and which layers it "
	                  "collides with (maskBits) - see physics.CollisionLayers for allocating layer bits by name.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "isSensor", wren_PhysicsBodyGetIsSensor,
	                  "Gets whether this body is a sensor (reports collisionEnter/Exit but pushes nothing)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "isSensor=(_)", wren_PhysicsBodySetIsSensor,
	                  "Sets whether this body is a sensor (reports collisionEnter/Exit but pushes nothing)");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "PhysicsBody", "create(_,_,_)", wren_PhysicsBodyCreate,
	                  "Create a physics body");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "PhysicsBody", "get(_)", wren_PhysicsBodyGet,
	                  "Gets a physics body");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "PhysicsBody", "setLinearVelocity(_,_)",
	                  wren_PhysicsBodyStaticSetLinearVelocity, "Sets the linear velocity of a physics body.");
}
