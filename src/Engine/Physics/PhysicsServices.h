#pragma once

#include <memory>

#include "Engine/Physics/CollisionLayers.h"
#include "Engine/Physics/PhysicsWorld.h"

namespace Struktur::Physics
{
// The physics library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<CollisionLayers> collisionLayers;
	std::unique_ptr<PhysicsWorld> physicsWorld;
};
}  // namespace Struktur::Physics
