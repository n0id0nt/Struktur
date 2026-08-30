#pragma once

#include <cstdint>

#include "box2d/box2d.h"

namespace Struktur
{
namespace Component
{
struct PhysicsBody
{
	b2Body* body     = nullptr;
	bool isKinematic = false;

	// Which Transform::version was last pushed into the Box2D body (see PhysicsSystem::SyncTransformsToPhysics).
	// Deliberately separate from Transform::dirty - that flag is a shared, general-purpose "world cache is
	// stale" signal that ANY system reading GetWorldPosition/GetWorldScale/GetWorldRotation clears as a side
	// effect (TransformSystem::EnsureWorldUpToDate), so relying on it here meant some other system reading this
	// entity's position before PhysicsSystem got its turn would silently steal the "needs sync" signal - the
	// body would never receive the correct position, and PhysicsSystem::SyncPhysicsToTransforms's own
	// unconditional pull-back would then overwrite the (correctly cached, but never pushed) Transform with the
	// body's stale position, permanently. Comparing against Transform::version instead - a counter that only
	// ever increments on a real change, never on a read - can't be stolen this way. Starts mismatched against a
	// fresh Transform's version (0) so a newly created body always syncs once on its first opportunity.
	uint32_t syncedTransformVersion = 0xFFFFFFFFu;

	// The body's position/angle as of the start of this frame's fixed-step batch (see
	// PhysicsSystem::SnapshotPreviousTransforms) - the "from" side of the render-time blend
	// PhysicsSystem::SyncPhysicsToTransforms does between this and the body's current (post-step) position, so a
	// render frame that lands between two fixed steps shows smooth motion instead of the body's position visibly
	// holding still then jumping (fixed steps don't run every render frame - see GameLoop's accumulator loop).
	// Reset to match the current position whenever SyncTransformsToPhysics pushes an external move (a script
	// teleporting something) so that snap is instant rather than smeared across the next render frame too.
	b2Vec2 previousPositionMeters = b2Vec2(0.0f, 0.0f);
	float previousAngleRadians    = 0.0f;
};
}  // namespace Component
}  // namespace Struktur
