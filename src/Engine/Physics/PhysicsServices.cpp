#include "PhysicsServices.h"

namespace Struktur::Physics
{
Services::Services()
{
	collisionLayers = std::make_unique<CollisionLayers>();
	physicsWorld    = std::make_unique<PhysicsWorld>();
}

void Services::Shutdown()
{
	collisionLayers.reset();
	physicsWorld.reset();
}
}  // namespace Struktur::Physics
