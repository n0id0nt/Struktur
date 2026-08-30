#include "PhysicsSystem.h"

#include "Debug/Profiling/Profiler.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Util/MathUtil.h"
#include "Engine/World/TileMap.h"
#include "glm/common.hpp"
#include "glm/gtc/quaternion.hpp."

void Struktur::System::PhysicsSystem::Update(GameContext& context)
{
	// Every render frame, NOT fixed-rate - see this method's own declaration comment / PhysicsFixedStepSystem
	// for why the sync can't be delayed to the fixed cadence the way the actual Step() below is.
	PROFILE_BEGIN_SCOPE(syncToPhysics, "Sync to Physics");
	SyncTransformsToPhysics(context);
	PROFILE_END_SCOPE(syncToPhysics);
	PROFILE_BEGIN_SCOPE(syncFromPhysics, "Sync from Physics");
	SyncPhysicsToTransforms(context);
	PROFILE_END_SCOPE(syncFromPhysics);
}

void Struktur::System::PhysicsSystem::StepPhysics(GameContext& context, float deltaTime)
{
	// Called from PhysicsFixedStepSystem at GameData::timeStep cadence (see GameLoop's accumulator loop in
	// Game.cpp) - deliberately just the Box2D advance, no sync (see Update() above for why sync stays per-frame).
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	PROFILE_BEGIN_SCOPE(stepPhysics, "Step Physics");
	physicsWorld.Step(context, deltaTime);
	PROFILE_END_SCOPE(stepPhysics);
}

void Struktur::System::PhysicsFixedStepSystem::Update(GameContext& context)
{
	context.GetSystemManager().GetSystem<PhysicsSystem>().StepPhysics(context, context.GetGameData().timeStep);
}

void Struktur::System::PhysicsSystem::SyncPhysicsToTransforms(GameContext& context)
{
	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	TransformSystem& transformSystem    = context.GetSystemManager().GetSystem<TransformSystem>();
	Core::GameData& gameData            = context.GetGameData();

	auto view = registry.view<Component::PhysicsBody, Component::Transform>(entt::exclude<Inactive>);

	const float ppm = physicsWorld.GetPixelsPerMeter();

	// How far into the fixed step that HASN'T run yet this render frame currently sits, e.g. 0.4 = 40% of the
	// way toward a step that's still 60% of a timeStep away. Blending previous->current by this fraction (rather
	// than just showing "current", the body's position as of its last completed step) is what actually smooths
	// motion between fixed steps - see PhysicsBody::previousPositionMeters's own comment for why that gap exists
	// at all. Clamped defensively; by the accumulator loop's own invariant this is already in [0,1).
	float alpha = gameData.timeStep > 0.0f ? gameData.physicsAccumulator / gameData.timeStep : 0.0f;
	alpha       = glm::clamp(alpha, 0.0f, 1.0f);

	PROFILE_SCOPE("Sync All From Physics");
	for (auto [entity, physicsBody, transform] : view.each())
	{
		if (physicsBody.body)
		{
			// Get world position from physics
			PROFILE_BEGIN_SCOPE(physicsPosition, "Get Phyics Position");
			b2Vec2 currentPosition = physicsBody.body->GetPosition();
			b2Vec2 position(glm::mix(physicsBody.previousPositionMeters.x, currentPosition.x, alpha),
			                glm::mix(physicsBody.previousPositionMeters.y, currentPosition.y, alpha));
			float angle     = glm::mix(physicsBody.previousAngleRadians, physicsBody.body->GetAngle(), alpha);

			glm::vec3 scale = transformSystem.GetWorldScale(context, entity);

			glm::vec3 worldPos(position.x * ppm, position.y * ppm, 0.0f);
			glm::quat worldAngle = glm::angleAxis(angle, glm::vec3(0, 0, 1));
			PROFILE_END_SCOPE(physicsPosition);
			PROFILE_BEGIN_SCOPE(setTransform, "Set Transform");
			transformSystem.SetWorldTransformDirect(context, entity, worldPos, scale, worldAngle);
			PROFILE_END_SCOPE(setTransform);

			// SetWorldTransformDirect falls back to the full SetLocalTransform (and so bumps Transform::version)
			// for any entity with a parent - which includes almost everything here, since GameObjectManager
			// parents most entities somewhere under a world/level root. Left unmarked, that version bump would
			// look to SyncTransformsToPhysics exactly like a real external move, and it would push this same
			// value straight back into the body next call - for a dynamic body like the player, that overwrites
			// Box2D's own velocity-integrated position with the one-step-stale value just read here, freezing
			// it in place. This value came FROM physics, so physics is already caught up to it - record that
			// immediately, the same way SyncTransformsToPhysics does after an actual push.
			physicsBody.syncedTransformVersion = transform.version;
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
		// Compared against Transform::version, not Transform::dirty - see PhysicsBody::syncedTransformVersion's
		// own comment for why the shared, reader-consumable dirty flag isn't safe to gate this on.
		if (physicsBody.body && transform.version != physicsBody.syncedTransformVersion)
		{
			PROFILE_BEGIN_SCOPE(convertAngle, "Convert Angle");
			glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);
			glm::quat worldRotation = transformSystem.GetWorldRotation(context, entity);
			float angleZ            = Struktur::Util::Math::AngleZFromQuat(worldRotation);
			PROFILE_END_SCOPE(convertAngle);
			// create helper functions to convert to and from b2vec to glm::vec2 using hte physics scale
			PROFILE_BEGIN_SCOPE(uploadToPhysics, "Upload To Physics");
			b2Vec2 pushedPosition(worldPosition.x * metersPerPixel, worldPosition.y * metersPerPixel);
			physicsBody.body->SetTransform(pushedPosition, angleZ);
			PROFILE_END_SCOPE(uploadToPhysics);
			physicsBody.syncedTransformVersion = transform.version;

			// An external move (a script teleporting this entity) should read as an instant snap, not a glide -
			// without this, the next SyncPhysicsToTransforms would blend from wherever this body was as of the
			// last fixed step all the way to the teleported position, smearing the jump across a render frame.
			physicsBody.previousPositionMeters = pushedPosition;
			physicsBody.previousAngleRadians   = angleZ;
		}
	}
}

void Struktur::System::PhysicsSystem::SnapshotPreviousTransforms(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();

	auto view = registry.view<Component::PhysicsBody>(entt::exclude<Inactive>);

	PROFILE_SCOPE("Snapshot Previous Physics Transforms");
	for (auto [entity, physicsBody] : view.each())
	{
		if (physicsBody.body)
		{
			physicsBody.previousPositionMeters = physicsBody.body->GetPosition();
			physicsBody.previousAngleRadians   = physicsBody.body->GetAngle();
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

	// Starts previous == current (its spawn position) so its first render-time blend (see
	// SyncPhysicsToTransforms) has nothing to interpolate away from - without this it would default to (0,0)
	// and visibly flash at the origin for the one frame before SnapshotPreviousTransforms next runs.
	physicsBody.previousPositionMeters = body->GetPosition();
	physicsBody.previousAngleRadians   = body->GetAngle();

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

	// See the shape-taking overload's own comment above for why.
	physicsBody.previousPositionMeters = body->GetPosition();
	physicsBody.previousAngleRadians   = body->GetAngle();

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
	filter.maskBits     = maskBits;

	for (b2Fixture* fixture = physicsBody.body->GetFixtureList(); fixture; fixture = fixture->GetNext())
	{
		fixture->SetFilterData(filter);
	}
}
