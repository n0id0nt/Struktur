#include "PhysicsSystem.h"

#include "Debug/Profiling/Profiler.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/World/TileMap.h"
#include "Engine/GameContext.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Util/MathUtil.h"
#include "glm/gtc/quaternion.hpp."

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
	physicsWorld.Step(context, deltaTime);
	PROFILE_END_SCOPE(stepPhysics);
	PROFILE_BEGIN_SCOPE(syncFromPhysics, "Sync from Physics");
	SyncPhysicsToTransforms(context);
	PROFILE_END_SCOPE(syncFromPhysics);
}

void Struktur::System::PhysicsSystem::SyncPhysicsToTransforms(GameContext& context)
{
	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	TransformSystem& transformSystem    = context.GetSystemManager().GetSystem<TransformSystem>();

	auto view = registry.view<Component::PhysicsBody, Component::Transform>(entt::exclude<Inactive>);

	const float ppm = physicsWorld.GetPixelsPerMeter();

	PROFILE_SCOPE("Sync All From Physics");
	for (auto [entity, physicsBody, transform] : view.each())
	{
		if (physicsBody.body)
		{
			// Get world position from physics
			PROFILE_BEGIN_SCOPE(physicsPosition, "Get Phyics Position");
			b2Vec2 position = physicsBody.body->GetPosition();
			float angle     = physicsBody.body->GetAngle();

			glm::vec3 scale = transformSystem.GetWorldScale(context, entity);

			glm::vec3 worldPos(position.x * ppm, position.y * ppm, 0.0f);
			glm::quat worldAngle = glm::angleAxis(angle, glm::vec3(0, 0, 1));
			PROFILE_END_SCOPE(physicsPosition);
			PROFILE_BEGIN_SCOPE(setTransform, "Set Transform");
			transformSystem.SetWorldTransformDirect(context, entity, worldPos, scale, worldAngle);
			PROFILE_END_SCOPE(setTransform);
		}
	}
}

void Struktur::System::PhysicsSystem::SyncTransformsToPhysics(GameContext& context)
{
	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	TransformSystem& transformSystem    = context.GetSystemManager().GetSystem<TransformSystem>();

	auto view = registry.view<Component::PhysicsBody, Component::Transform>(entt::exclude<Inactive>);

	const float ppm            = physicsWorld.GetPixelsPerMeter();
	const float metersPerPixel = 1.0f / ppm;

	PROFILE_SCOPE("Sync All To Physics");
	for (auto [entity, physicsBody, transform] : view.each())
	{
		if (physicsBody.body && transform.dirty)
		{
			PROFILE_BEGIN_SCOPE(convertAngle, "Convert Angle");
			glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);
			glm::quat worldRotation = transformSystem.GetWorldRotation(context, entity);
			float angleZ            = Struktur::Util::Math::AngleZFromQuat(worldRotation);
			PROFILE_END_SCOPE(convertAngle);
			// create helper functions to convert to and from b2vec to glm::vec2 using hte physics scale
			PROFILE_BEGIN_SCOPE(uploadToPhysics, "Upload To Physics");
			physicsBody.body->SetTransform(
			    b2Vec2(worldPosition.x * metersPerPixel, worldPosition.y * metersPerPixel), angleZ);
			PROFILE_END_SCOPE(uploadToPhysics);
		}
	}
}

Struktur::Component::PhysicsBody& Struktur::System::PhysicsSystem::CreatePhysicsBody(GameContext& context,
                                                                                     entt::entity entity,
                                                                                     const b2BodyDef& bodyDef,
                                                                                     const b2Shape& shape)
{
	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	b2Body* body                = physicsWorld.CreateBody(&bodyDef);
	body->GetUserData().pointer = static_cast<uintptr_t>(entity);

	b2FixtureDef fixtureDef;
	fixtureDef.shape       = &shape;
	fixtureDef.density     = 1.f;
	fixtureDef.friction    = 0.4;
	fixtureDef.restitution = 0.f;

	body->CreateFixture(&fixtureDef);

	Component::PhysicsBody& physicsBody =
	    registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);

	return physicsBody;
}

Struktur::Component::PhysicsBody& Struktur::System::PhysicsSystem::CreatePhysicsBody(GameContext& context,
                                                                                     entt::entity entity,
                                                                                     const b2BodyDef& bodyDef)
{
	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	b2Body* body                = physicsWorld.CreateBody(&bodyDef);
	body->GetUserData().pointer = static_cast<uintptr_t>(entity);

	Component::PhysicsBody& physicsBody =
	    registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);

	return physicsBody;
}

void Struktur::System::PhysicsSystem::SetCollisionFilter(Component::PhysicsBody& physicsBody, uint16_t categoryBits,
                                                          uint16_t maskBits)
{
	if (!physicsBody.body)
	{
		return;
	}

	b2Filter filter;
	filter.categoryBits = categoryBits;
	filter.maskBits      = maskBits;

	for (b2Fixture* fixture = physicsBody.body->GetFixtureList(); fixture; fixture = fixture->GetNext())
	{
		fixture->SetFilterData(filter);
	}
}
