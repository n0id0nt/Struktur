#include "PhysicsSystem.h"

#include "glm/gtc/quaternion.hpp."

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Game/TileMap.h"
#include "Debug/Profiling/Profiler.h"
#ifdef DEBUG
#include <format>
#endif

void Struktur::System::PhysicsSystem::Update(GameContext& context)
{
	float deltaTime = context.GetTimeSystem().scaledDelta;
	StepPhysics(context, deltaTime);
}

void Struktur::System::PhysicsSystem::StepPhysics(GameContext& context, float deltaTime)
{
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	PROFILE_BEGIN_SCOPE(syncToPhysics, "Sync to Physics");
	SyncTransformsToPhysics(context);
	PROFILE_END_SCOPE(syncToPhysics);
	PROFILE_BEGIN_SCOPE(stepPhysics, "Step Physics");
	physicsWorld.Step(deltaTime);
	PROFILE_END_SCOPE(stepPhysics);
	PROFILE_BEGIN_SCOPE(syncFromPhysics, "Sync from Physics");
	SyncPhysicsToTransforms(context);
	PROFILE_END_SCOPE(syncFromPhysics);
}

void Struktur::System::PhysicsSystem::SyncPhysicsToTransforms(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	TransformSystem& transformSystem = context.GetSystemManager().GetSystem<TransformSystem>();

	auto view = registry.view<Component::PhysicsBody, Component::LocalTransform>();

	for (auto [entity, physicsBody, transform] : view.each())
	{
#ifdef DEBUG
		PROFILE_SCOPE(std::format("Sync Entity {} From Physics", (int)entity));
#endif
		if (physicsBody.body && physicsBody.syncFromPhysics)
		{
			// Get world position from physics
			PROFILE_BEGIN_SCOPE(physicsPosition, "Get Phyics Position");
			b2Vec2 position = physicsBody.body->GetPosition();
			float angle = physicsBody.body->GetAngle();

			glm::vec3 scale = glm::vec3(1.0f);
			if (auto* worldTransform = registry.try_get<Component::WorldTransform>(entity))
			{
				scale = worldTransform->scale;
			}

			glm::vec3 worldPos(position.x * physicsWorld.GetPixelsPerMeter(), position.y * physicsWorld.GetPixelsPerMeter(), 0.0f);
			glm::quat worldAngle = glm::angleAxis(angle, glm::vec3(0, 0, 1));
			PROFILE_END_SCOPE(physicsPosition);
			PROFILE_BEGIN_SCOPE(setTransform, "Set Transform");
			transformSystem.SetWorldTransform(context, entity, worldPos, scale, worldAngle);
			PROFILE_END_SCOPE(setTransform);
		}
	}
}

void Struktur::System::PhysicsSystem::SyncTransformsToPhysics(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	auto view = registry.view<Component::PhysicsBody, Component::LocalTransform, Component::WorldTransform>();

	for (auto [entity, physicsBody, transform, worldTransform] : view.each())
	{
#ifdef DEBUG
		PROFILE_SCOPE(std::format("Sync Entity {} To Physics", (int)entity));
#endif
		if (physicsBody.body && physicsBody.syncToPhysics)
		{
			PROFILE_BEGIN_SCOPE(convertAngle, "Convert Angle");
			glm::vec3 euler = glm::eulerAngles(worldTransform.rotation);
			PROFILE_END_SCOPE(convertAngle);
			// create helper functions to convert to and from b2vec to glm::vec2 using hte physics scale
			PROFILE_BEGIN_SCOPE(uploadToPhysics, "Upload To Physics");
			physicsBody.body->SetTransform(b2Vec2(worldTransform.position.x / physicsWorld.GetPixelsPerMeter(), worldTransform.position.y / physicsWorld.GetPixelsPerMeter()), euler.z);
			PROFILE_END_SCOPE(uploadToPhysics);
		}
	}
}

Struktur::Component::PhysicsBody& Struktur::System::PhysicsSystem::CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef, const b2Shape& shape)
{
	entt::registry& registry = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	b2Body* body = physicsWorld.CreateBody(&bodyDef);
	body->GetUserData().pointer = static_cast<uintptr_t>(entity);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.f;
	fixtureDef.friction = 0.4;
	fixtureDef.restitution = 0.f;

	body->CreateFixture(&fixtureDef);

	Component::PhysicsBody& physicsBody = registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);

	return physicsBody;
}

Struktur::Component::PhysicsBody& Struktur::System::PhysicsSystem::CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef)
{
	entt::registry& registry = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	b2Body* body = physicsWorld.CreateBody(&bodyDef);
	body->GetUserData().pointer = static_cast<uintptr_t>(entity);

	Component::PhysicsBody& physicsBody = registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);

	return physicsBody;
}
