#include "ContactListener.h"

#include <format>
#include <limits>

#include "Debug/Assertions.h"
#include "Engine/Callback/Variant.h"
#include "Engine/Event/EventManager.h"
#include "Engine/GameContext.h"

void Struktur::Physics::ContactListener::BeginContact(b2Contact* contact)
{
	if (!contact->IsTouching())
	{
		return;
	}

	entt::entity entityA = entt::null;
	entt::entity entityB = entt::null;
	GetContactEntities(contact, entityA, entityB);

	PublishContactEvent("collisionEnter", contact, entityA, entityB);
}

void Struktur::Physics::ContactListener::EndContact(b2Contact* contact)
{
	entt::entity entityA = entt::null;
	entt::entity entityB = entt::null;
	GetContactEntities(contact, entityA, entityB);

	PublishContactEvent("collisionExit", contact, entityA, entityB);
}

void Struktur::Physics::ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	// Intentionally not pushed onto the messaging system, unlike BeginContact/EndContact - PreSolve fires on
	// every step a contact is still touching, not just on the enter/exit transition, so two overlapping bodies
	// sitting still would flood the once-per-frame-drained event queue every single frame. Add a dedicated,
	// non-queued hook here instead if per-step reactive logic (e.g. conditionally disabling a contact via
	// contact->SetEnabled) is ever actually needed.
}

void Struktur::Physics::ContactListener::GetContactEntities(b2Contact* contact, entt::entity& entityA,
                                                            entt::entity& entityB)
{
	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();

	entityA = static_cast<entt::entity>(fixtureA->GetBody()->GetUserData().pointer);
	entityB = static_cast<entt::entity>(fixtureB->GetBody()->GetUserData().pointer);
}

void Struktur::Physics::ContactListener::PublishContactEvent(const char* type, b2Contact* contact,
                                                              entt::entity entityA, entt::entity entityB)
{
	if (!m_context)
	{
		return;
	}

	// Reuses Contact's own manifold/velocity extraction below - the fixture/otherFixture perspective passed in
	// here is arbitrary since this event is symmetric (both entityA and entityB go in the payload, not just "the
	// other one"), it's only used to compute normal/velocity once for both sides.
	Contact contactInfo(contact->GetFixtureA(), contact->GetFixtureB(), entityB, contact);

	Callback::VariantMap data;
	data.items["entityA"]  = static_cast<int>(entityA);
	data.items["entityB"]  = static_cast<int>(entityB);
	data.items["normal"]   = contactInfo.normal;
	data.items["velocity"] = contactInfo.velocity;

	DEBUG_INFO(
	    std::format("Physics contact '{}' between entity {} and entity {}", type, (int)entityA, (int)entityB).c_str());
	m_context->GetEventManager().AddEvent(type, data);
}

Struktur::Physics::ContactListener::Contact::Contact(b2Fixture* fixture, b2Fixture* otherFixture, entt::entity other,
                                                     b2Contact* contact)
{
	contactPoints.clear();
	impulses.clear();

	b2WorldManifold worldManifold;
	contact->GetWorldManifold(&worldManifold);
	b2Manifold* manifold = contact->GetManifold();

	for (int i = 0; i < manifold->pointCount; ++i)
	{
		b2Vec2 point = worldManifold.points[i];
		contactPoints.push_back(glm::vec2(point.x, point.y));
		impulses.push_back(manifold->points[i].normalImpulse);
	}

	b2Body* bodyA           = contact->GetFixtureA()->GetBody();
	b2Body* bodyB           = contact->GetFixtureB()->GetBody();
	b2Vec2 relativeVelocity = bodyB->GetLinearVelocity() - bodyA->GetLinearVelocity();

	b2Vec2 normal = worldManifold.normal;

	this->other        = other;
	this->velocity     = glm::vec2{relativeVelocity.x, relativeVelocity.y};
	this->normal       = glm::vec2{normal.x, normal.y};
	this->fixture      = fixture;
	this->otherFixture = otherFixture;
	this->enabled      = true;
}
