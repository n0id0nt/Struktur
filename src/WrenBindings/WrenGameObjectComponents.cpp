#include "wrenGameObjectComponents.h"

#include <algorithm>

#include "ComponentListXMacro.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/Scripting/WrenUtil.h"
#include "Engine/Util/Color.h"
#include "WrenMath.h"
#include "WrenPhysics.h"
#include "WrenResourceManager.h"
#include "wren.hpp"

// ============================================================================
// COMPONENT ALLOCATOR BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_type, component_name_string) \
	void wren_##component_name##Allocate(WrenVM* vm)                     \
	{                                                                    \
		wrenSetSlotNewForeign(vm, 0, 0, sizeof(Wren##component_name));   \
	}
COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// COMPONENT FINALIZER BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_type, component_name_string) \
	void wren_##component_name##Finalize(void* data)                     \
	{                                                                    \
		Wren##component_name* value = (Wren##component_name*)data;       \
		value->~Wren##component_name();                                  \
	}
COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// COMPONENT GET BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_type, component_name_string)                              \
	void wren_##component_name##Get(WrenVM* vm)                                                       \
	{                                                                                                 \
		Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));    \
		entt::registry& registry       = context->GetRegistry();                                      \
		double entityId                = wrenGetSlotDouble(vm, 1);                                    \
		entt::entity entity            = static_cast<entt::entity>(entityId);                         \
		Struktur::Component::component_type* component =                                              \
		    registry.try_get<Struktur::Component::component_type>(entity);                            \
		if (component)                                                                                \
		{                                                                                             \
			wrenGetVariable(vm, "gameObjectComponents", component_name_string, 1);                    \
			Wren##component_name* wrenComponent =                                                     \
			    (Wren##component_name*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(Wren##component_name)); \
			new (wrenComponent) Wren##component_name(entity, component);                              \
		}                                                                                             \
		else                                                                                          \
		{                                                                                             \
			wrenSetSlotNull(vm, 0);                                                                   \
		}                                                                                             \
	}
COMPONENT_LIST
#undef COMPONENT

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
// LOCAL TRANSFORM BINDINGS
// ============================================================================

// LocalTransform.getPosition() -> vec3 or null
void wren_LocalTransformGetPosition(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->localPosition);
}

// LocalTransform.getPosition(entity) -> vec3 or null
void wren_LocalTransformStaticGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->localPosition);
}

// LocalTransform.setPosition(vec3)
void wren_LocalTransformSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, vec->value, transform->component->localScale,
	                                  transform->component->localRotation);
}

// LocalTransform.setPosition(entity, vec3)
void wren_LocalTransformStaticSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, vec->value, glm::vec3(1.0f),
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, vec->value, transform->localScale, transform->localRotation);
}

// LocalTransform.getScale() -> vec3 or null
void wren_LocalTransformGetScale(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->localScale);
}

// LocalTransform.getScale(entity) -> vec3 or null
void wren_LocalTransformStaticGetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->localScale);
}

// LocalTransform.setScale(vec3)
void wren_LocalTransformSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, transform->component->localPosition, vec->value,
	                                  transform->component->localRotation);
}

// LocalTransform.setScale(entity, vec3)
void wren_LocalTransformStaticSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), vec->value,
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, transform->localPosition, vec->value, transform->localRotation);
}

// LocalTransform.getRotation() -> Quat or null
void wren_LocalTransformGetRotation(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->component->localRotation);
}

// LocalTransform.getRotation(entity) -> Quat or null
void wren_LocalTransformStaticGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->localRotation);
}

// LocalTransform.setRotation(Quat)
void wren_LocalTransformSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, transform->component->localPosition,
	                                  transform->component->localScale, quat->value);
}

// LocalTransform.setPosition(entity, Quat)
void wren_LocalTransformStaticSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), glm::vec3(1.0f), quat->value);
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, transform->localPosition, transform->localScale, quat->value);
}

// LocalTransform.getMatrix() -> Mat4 or null
void wren_LocalTransformGetMatrix(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->component->localMatrix);
}

// LocalTransform.getMatrix(entity) -> Mat4 or null
void wren_LocalTransformStaticGetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->localMatrix);
}

// LocalTransform.setMatrix(mat4)
void wren_LocalTransformSetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	if (!mat)
	{
		DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, mat->value);
}

// LocalTransform.setMatrix(entity, mat4)
void wren_LocalTransformStaticSetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	if (!mat)
	{
		DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, mat->value);
}

// ============================================================================
// WORLD TRANSFORM BINDINGS
// ============================================================================

// WorldTransform.getPosition() -> vec3 or null
void wren_WorldTransformGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldPosition(*context, transform->entity));
}

// WorldTransform.getPosition(entity) -> vec3 or null
void wren_WorldTransformStaticGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldPosition(*context, entity));
}

// WorldTransform.setPosition(vec3)
void wren_WorldTransformSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity, vec->value,
	                                  transformSystem.GetWorldScale(*context, transform->entity),
	                                  transformSystem.GetWorldRotation(*context, transform->entity));
}

// WorldTransform.setPosition(entity, vec3)
void wren_WorldTransformStaticSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, vec->value, glm::vec3(1.0f),
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, vec->value, transformSystem.GetWorldScale(*context, entity),
	                                  transformSystem.GetWorldRotation(*context, entity));
}

// WorldTransform.getScale() -> vec3 or null
void wren_WorldTransformGetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldScale(*context, transform->entity));
}

// WorldTransform.getScale(entity) -> vec3 or null
void wren_WorldTransformStaticGetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldScale(*context, entity));
}

// WorldTransform.setScale(vec3)
void wren_WorldTransformSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity,
	                                  transformSystem.GetWorldPosition(*context, transform->entity), vec->value,
	                                  transformSystem.GetWorldRotation(*context, transform->entity));
}

// WorldTransform.setScale(entity, vec3)
void wren_WorldTransformStaticSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), vec->value,
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, transformSystem.GetWorldPosition(*context, entity), vec->value,
	                                  transformSystem.GetWorldRotation(*context, entity));
}

// WorldTransform.getRotation() -> Quat or null
void wren_WorldTransformGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transformSystem.GetWorldRotation(*context, transform->entity));
}

// WorldTransform.getRotation(entity) -> Quat or null
void wren_WorldTransformStaticGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transformSystem.GetWorldRotation(*context, entity));
}

// WorldTransform.setRotation(Quat)
void wren_WorldTransformSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity,
	                                  transformSystem.GetWorldPosition(*context, transform->entity),
	                                  transformSystem.GetWorldScale(*context, transform->entity), quat->value);
}

// WorldTransform.setPosition(entity, Quat)
void wren_WorldTransformStaticSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), glm::vec3(1.0f), quat->value);
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, transformSystem.GetWorldPosition(*context, entity),
	                                  transformSystem.GetWorldScale(*context, entity), quat->value);
}

// WorldTransform.getMatrix() -> Mat4 or null
void wren_WorldTransformGetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "math", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transformSystem.GetWorldMatrix(*context, transform->entity));
}

// WorldTransform.getMatrix(entity) -> Mat4 or null
void wren_WorldTransformStaticGetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transformSystem.GetWorldMatrix(*context, entity));
}

// WorldTransform.setMatrix(mat4)
void wren_WorldTransformSetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	if (!mat)
	{
		DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity, mat->value);
}

// WorldTransform.setMatrix(entity, mat4)
void wren_WorldTransformStaticSetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	if (!mat)
	{
		DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, mat->value);
}

// ============================================================================
// SCRIPT BINDINGS
// ============================================================================

// Script.create(entity, className) -> ScriptInstance
void wren_ScriptCreate(WrenVM* vm)
{
	entt::registry& registry = static_cast<Struktur::GameContext*>(wrenGetUserData(vm))->GetRegistry();

	entt::entity entity   = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* className = wrenGetSlotString(vm, 2);

	// GameObjectManager::OnScriptConstruct initialises the script and queues its Start() call.
	registry.emplace<Struktur::Component::WrenScript>(entity, className);
}

// Script.createArgPairs(entity, className, pairs) -> ScriptInstance
// Internal: `pairs` is the MapUtil.mapToPairs encoding of the arg map. Wren has no notion of a
// private method, so this is just a plain method - prefer the public Script.createArg(entity,
// className, map) Wren-side wrapper below, which calls through to this.
void wren_ScriptCreateArgPairs(WrenVM* vm)
{
	entt::registry& registry = static_cast<Struktur::GameContext*>(wrenGetUserData(vm))->GetRegistry();

	entt::entity entity   = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* className = wrenGetSlotString(vm, 2);
	wrenEnsureSlots(vm, 6);
	std::vector<Struktur::Wren::WrenItem> wrenArgs = Struktur::Wren::Util::GetWrenMapFromPairs(vm, 3, 4);

	// GameObjectManager::OnScriptConstruct initialises the script and queues its Start() call.
	registry.emplace<Struktur::Component::WrenScript>(entity, className, wrenArgs);
}

// Script.isInitialised -> bool
void wren_ScriptIsInitialised(WrenVM* vm)
{
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
		DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotBool(vm, 0, script->component->isInitialised);
}

// Script.hasError -> bool
void wren_ScriptHasError(WrenVM* vm)
{
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
		DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotBool(vm, 0, script->component->hasError);
}

// Script.getErrorMessage -> string
void wren_ScriptGetErrorMessage(WrenVM* vm)
{
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
		DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotString(vm, 0, script->component->errorMessage.c_str());
}

// Script.getInstance() -> classHandle or null
void wren_ScriptGetInstance(WrenVM* vm)
{
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
		DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (script->component->hasError)
	{
		// DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (!script->component->isInitialised)
	{
		DEBUG_WARNING("Script.get: Entity's script is not initialised");
		wrenSetSlotNull(vm, 0);
		return;
	}
	// Get the script's instance
	if (!script->component->instanceHandle)
	{
		DEBUG_ERROR("Script.get: No instance handle");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotHandle(vm, 0, script->component->instanceHandle);
}

// Script.getInstance(entity) -> classHandle or null
void wren_ScriptStaticGetInstance(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* script = registry.try_get<Struktur::Component::WrenScript>(entity);
	if (!script)
	{
		DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (script->hasError)
	{
		// DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (!script->isInitialised)
	{
		DEBUG_WARNING("Script.get: Entity's script is not initialised");
		wrenSetSlotNull(vm, 0);
		return;
	}
	// Get the script's instance
	if (!script->instanceHandle)
	{
		DEBUG_ERROR("Script.get: No instance handle");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotHandle(vm, 0, script->instanceHandle);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(GameObjectComponent)
{
	// Register BodyDefinition foreign class
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

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "LocalTransform", wren_LocalTransformAllocate,
	                   wren_LocalTransformFinalize, "Sprite animation class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "position", wren_LocalTransformGetPosition,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "position=(_)",
	                  wren_LocalTransformSetPosition, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "rotation", wren_LocalTransformGetRotation,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "rotation=(_)",
	                  wren_LocalTransformSetRotation, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "scale", wren_LocalTransformGetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "scale=(_)", wren_LocalTransformSetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "matrix", wren_LocalTransformGetMatrix,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "matrix=(_)", wren_LocalTransformSetMatrix,
	                  "Sets the sprites texture");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "get(_)", wren_LocalTransformGet,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getPosition(_)",
	                  wren_LocalTransformStaticGetPosition,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setPosition(_,_)",
	                  wren_LocalTransformStaticSetPosition, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getRotation(_)",
	                  wren_LocalTransformStaticGetRotation, "Get rotation of entity as Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setRotation(_,_)",
	                  wren_LocalTransformStaticSetRotation, "Set rotation of entity from Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getScale(_)",
	                  wren_LocalTransformStaticGetScale,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setScale(_,_)",
	                  wren_LocalTransformStaticSetScale, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getMatrix(_)",
	                  wren_LocalTransformStaticGetMatrix,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setMatrix(_,_)",
	                  wren_LocalTransformStaticSetMatrix, "Set the position of an entity.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "WorldTransform", wren_WorldTransformAllocate,
	                   wren_WorldTransformFinalize, "Sprite animation class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "position", wren_WorldTransformGetPosition,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "position=(_)",
	                  wren_WorldTransformSetPosition, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "rotation", wren_WorldTransformGetRotation,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "rotation=(_)",
	                  wren_WorldTransformSetRotation, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "scale", wren_WorldTransformGetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "scale=(_)", wren_WorldTransformSetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "matrix", wren_WorldTransformGetMatrix,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "matrix=(_)", wren_WorldTransformSetMatrix,
	                  "Sets the sprites texture");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "get(_)", wren_WorldTransformGet,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getPosition(_)",
	                  wren_WorldTransformStaticGetPosition,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setPosition(_,_)",
	                  wren_WorldTransformStaticSetPosition, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getRotation(_)",
	                  wren_WorldTransformStaticGetRotation, "Get rotation of entity as Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setRotation(_,_)",
	                  wren_WorldTransformStaticSetRotation, "Set rotation of entity from Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getScale(_)",
	                  wren_WorldTransformStaticGetScale,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setScale(_,_)",
	                  wren_WorldTransformStaticSetScale, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getMatrix(_)",
	                  wren_WorldTransformStaticGetMatrix,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setMatrix(_,_)",
	                  wren_WorldTransformStaticSetMatrix, "Set the position of an entity.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "Script", wren_WrenScriptAllocate, wren_WrenScriptFinalize,
	                   "Script class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "getInstance()", wren_ScriptGetInstance,
	                  "Gets a script instance");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "isInitialised", wren_ScriptIsInitialised,
	                  "Checks if script is initialised");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "hasError", wren_ScriptHasError,
	                  "Checks if script has error");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "errorMessage", wren_ScriptGetErrorMessage,
	                  "Gets Scripts error message");

	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Script", "create(_,_)", wren_ScriptCreate,
	                  "Creates the script Component.");
	WREN_CLASS_STATIC(
	    registry, "gameObjectComponents", "Script", "createArgPairs(_,_,_)", wren_ScriptCreateArgPairs,
	    "Internal: creates the script Component from a MapUtil-encoded arg map. Prefer createArg(_,_,_).");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Script", "get(_)", wren_WrenScriptGet,
	                  "Gets the script component");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Script", "getInstance(_)", wren_ScriptStaticGetInstance,
	                  "Gets a script instance");

	WREN_IMPORT(registry, "gameObjectComponents", "serialisation", "MapUtil");
	WREN_IMPL(registry, "gameObjectComponents", "Script", true,
	          "static createArg(entity, className, args) {\n"
	          "    return createArgPairs(entity, className, MapUtil.mapToPairs(args))\n"
	          "}");
}
