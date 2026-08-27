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
};
}  // namespace Component
}  // namespace Struktur
