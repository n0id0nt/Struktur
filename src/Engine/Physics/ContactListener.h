#pragma once

#include <vector>

#include "box2d/box2d.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Physics
{
class ContactListener : public b2ContactListener
{
public:
	struct Contact
	{
		Contact(b2Fixture* fixture, b2Fixture* otherFixture, entt::entity other, b2Contact* contact);

		b2Fixture* fixture;
		b2Fixture* otherFixture;
		entt::entity other;
		std::vector<glm::vec2> contactPoints;
		std::vector<float> impulses;
		glm::vec2 velocity;
		glm::vec2 normal;
		bool enabled;
	};

	// Only valid for the duration of a single PhysicsWorld::Step() call - set right before b2World::Step() runs
	// (see PhysicsWorld::Step), since Begin/EndContact/PreSolve only ever fire synchronously from within it.
	void SetContext(GameContext* context)
	{
		m_context = context;
	}

	void BeginContact(b2Contact* contact) override;
	void EndContact(b2Contact* contact) override;

	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

private:
	// Resolves both fixtures' owning entities via body user-data - see PhysicsSystem::CreatePhysicsBody, the sole
	// place bodies are created in this codebase, which always sets this before a body can generate a contact.
	void GetContactEntities(b2Contact* contact, entt::entity& entityA, entt::entity& entityB);
	// Pushes a single {entityA, entityB, normal, velocity} event onto the messaging system (see
	// GameContext::GetEventManager) for the given contact - both colliding entities receive it via EventSystem's
	// broadcast (see WrenScriptSystem::SendEvent), each able to tell which side it was on by comparing its own id
	// against entityA/entityB.
	void PublishContactEvent(const char* type, b2Contact* contact, entt::entity entityA, entt::entity entityB);

	GameContext* m_context = nullptr;
};
}  // namespace Physics
}  // namespace Struktur
