#pragma once

#include "Engine/ECS/SystemManager.h"
#include "box2d/box2d.h"
#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace Component
{
struct PhysicsBody;
}
namespace System
{
class PhysicsSystem : public ISystem
{
public:
	// Runs every render frame (AddUpdateSystem), NOT the fixed cadence - see PhysicsFixedStepSystem below for
	// why. Syncs Component::Transform <-> the Box2D body: pushes any Transform a script/level system marked
	// dirty this frame into the body, then pulls the body's current position back out for rendering.
	void Update(GameContext& context) override;

	// The actual Box2D simulation advance - deliberately NOT called from Update() above. Called instead via
	// PhysicsFixedStepSystem (AddFixedUpdateSystem) at GameData::timeStep cadence, independent of render rate.
	void StepPhysics(GameContext& context, float deltaTime);
	// Blends each body's PhysicsBody::previousPositionMeters/previousAngleRadians toward its current (post-step)
	// state using GameData::physicsAccumulator/timeStep as the blend factor, and writes the *blended* result into
	// Transform - see this method's own comment in the .cpp for why (render-frame smoothing between fixed steps).
	void SyncPhysicsToTransforms(GameContext& context);
	void SyncTransformsToPhysics(GameContext& context);

	// Captures every physics body's current position/angle as its new "previous" for this frame's render-time
	// blend (see SyncPhysicsToTransforms) - called once per frame, only when at least one fixed step is about to
	// run (see GameLoop's accumulator loop in Game.cpp), so a frame with zero steps keeps blending toward
	// whatever bracket the last stepped frame set up instead of collapsing previous==current and freezing.
	void SnapshotPreviousTransforms(GameContext& context);

	Component::PhysicsBody& CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef,
	                                          const b2Shape& shape);
	Component::PhysicsBody& CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef);

	// Applies categoryBits/maskBits (see Physics::CollisionLayers) to every fixture on this body - a body can
	// have more than one fixture, though today's CreatePhysicsBody only ever creates one.
	void SetCollisionFilter(Component::PhysicsBody& physicsBody, uint16_t categoryBits, uint16_t maskBits);

	std::string Name() const override
	{
		return "Physics System";
	}
};

// Registered via SystemManager::AddFixedUpdateSystem - reaches the already-constructed PhysicsSystem instance
// (via SystemManager::GetSystem, same pattern as WrenScriptFixedUpdateSystem) to run just the Box2D step at a
// fixed cadence, decoupled from PhysicsSystem::Update's per-frame Transform sync above. Splitting these two
// concerns across different cadences (rather than moving both to fixed-rate) matters: Component::Transform::dirty
// (see SyncTransformsToPhysics) is a general-purpose "world cache is stale" flag that gets cleared as a side
// effect the moment ANY system reads GetWorldPosition/GetWorldScale (TransformSystem::EnsureWorldUpToDate) - so
// a freshly spawned or moved entity's position must reach the physics body in the SAME frame it was set, before
// a render/camera system's read steals the flag, or the body silently keeps its stale/default position forever.
class PhysicsFixedStepSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Physics Fixed Step System";
	}
};
}  // namespace System
}  // namespace Struktur
