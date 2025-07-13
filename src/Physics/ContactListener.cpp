#include "ContactListener.h"

#include <limits>

void Struktur::Physics::ContactListener::BeginContact(b2Contact* contact)
{
	//if (!contact->IsTouching()) return;
	//
	//Entity* entityA = nullptr, * entityB = nullptr;
	//b2Fixture* fixtureA = contact->GetFixtureA(), * fixtureB = contact->GetFixtureB();
	//GetContactEntities(contact, entityA, entityB);
	//// call the collision events
	//bool collisionEnabled = true;
	//auto scriptComponentA = entityA->GetComponent<ScriptComponent>();
	//if (scriptComponentA)
	//{
	//	sol::state& lua = Engine::GetInstance()->GetSolState();
	//	Contact contactStruct(fixtureA, fixtureB, entityB, contact, &lua);
	//	scriptComponentA->Bind(lua); // TODO Might be a little slow to be rebinbinging and unbinding scripts on each collision event think up more efficent way
	//	scriptComponentA->OnCollisionEnter(contactStruct);
	//	scriptComponentA->Unbind(lua);
	//	collisionEnabled &= contactStruct.enabled;
	//}
	//auto scriptComponentB = entityB->GetComponent<ScriptComponent>();
	//if (scriptComponentB)
	//{
	//	sol::state& lua = Engine::GetInstance()->GetSolState();
	//	Contact contactStruct(fixtureB, fixtureA, entityA, contact, &lua);
	//	scriptComponentB->Bind(lua);
	//	scriptComponentB->OnCollisionEnter(contactStruct);
	//	scriptComponentB->Unbind(lua);
	//	collisionEnabled &= contactStruct.enabled;
	//}
	//contact->SetEnabled(collisionEnabled);
}

void Struktur::Physics::ContactListener::EndContact(b2Contact* contact)
{
	//Entity* entityA = nullptr, * entityB = nullptr;
	//b2Fixture* fixtureA = contact->GetFixtureA(), * fixtureB = contact->GetFixtureB();
	//GetContactEntities(contact, entityA, entityB);
	//// call the collision events
	//bool collisionEnabled = true;
	//auto scriptComponentA = entityA->GetComponent<ScriptComponent>();
	//if (scriptComponentA)
	//{
	//	sol::state& lua = Engine::GetInstance()->GetSolState();
	//	Contact contactStruct(fixtureA, fixtureB, entityB, contact, &lua);
	//	scriptComponentA->Bind(lua); // TODO Might be a little slow to be rebinbinging and unbinding scripts on each collision event think up more efficent way
	//	scriptComponentA->OnCollisionExit(contactStruct);
	//	scriptComponentA->Unbind(lua);
	//	collisionEnabled &= contactStruct.enabled;
	//}
	//auto scriptComponentB = entityB->GetComponent<ScriptComponent>();
	//if (scriptComponentB)
	//{
	//	sol::state& lua = Engine::GetInstance()->GetSolState();
	//	Contact contactStruct(fixtureB, fixtureA, entityA, contact, &lua);
	//	scriptComponentB->Bind(lua);
	//	scriptComponentB->OnCollisionExit(contactStruct);
	//	scriptComponentB->Unbind(lua);
	//	collisionEnabled &= contactStruct.enabled;
	//}
	//contact->SetEnabled(collisionEnabled);
}

void Struktur::Physics::ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	//Entity* entityA = nullptr, * entityB = nullptr;
	//b2Fixture* fixtureA = contact->GetFixtureA(), * fixtureB = contact->GetFixtureB();
	//GetContactEntities(contact, entityA, entityB);
	//// call the collision events
	//bool collisionEnabled = true;
	//auto scriptComponentA = entityA->GetComponent<ScriptComponent>();
	//if (scriptComponentA)
	//{
	//	sol::state& lua = Engine::GetInstance()->GetSolState();
	//	Contact contactStruct(fixtureA, fixtureB, entityB, contact, &lua);
	//	scriptComponentA->Bind(lua); // TODO Might be a little slow to be rebinbinging and unbinding scripts on each collision event think up more efficent way
	//	scriptComponentA->OnCollisionPreSolve(contactStruct);
	//	scriptComponentA->Unbind(lua);
	//	collisionEnabled &= contactStruct.enabled;
	//}
	//auto scriptComponentB = entityB->GetComponent<ScriptComponent>();
	//if (scriptComponentB)
	//{
	//	sol::state& lua = Engine::GetInstance()->GetSolState();
	//	Contact contactStruct(fixtureB, fixtureA, entityA, contact, &lua);
	//	scriptComponentB->Bind(lua);
	//	scriptComponentB->OnCollisionPreSolve(contactStruct);
	//	scriptComponentB->Unbind(lua);
	//	collisionEnabled &= contactStruct.enabled;
	//}
	//contact->SetEnabled(collisionEnabled);
}

void Struktur::Physics::ContactListener::GetContactEntities(b2Contact* contact, entt::entity entityA, entt::entity entityB)
{
	//// Identify fixtures involved in the collision
	//b2Fixture* fixtureA = contact->GetFixtureA();
	//b2Fixture* fixtureB = contact->GetFixtureB();
	//
	//// Use user data to get associated entities
	//entityA = reinterpret_cast<Entity*>(fixtureA->GetBody()->GetUserData().pointer);
	//entityB = reinterpret_cast<Entity*>(fixtureB->GetBody()->GetUserData().pointer);

}

Struktur::Physics::ContactListener::Contact::Contact(b2Fixture* fixture, b2Fixture* otherFixture, entt::entity other, b2Contact* contact)
{
	contactPoints.clear();
	impulses.clear();

	b2WorldManifold worldManifold;
	contact->GetWorldManifold(&worldManifold);
	b2Manifold* manifold = contact->GetManifold();

	for (int i = 0; i < manifold->pointCount; ++i) {
		b2Vec2 point = worldManifold.points[i];
		contactPoints.push_back(glm::vec2(point.x, point.y));
		impulses.push_back(manifold->points[i].normalImpulse);
	}

	b2Body* bodyA = contact->GetFixtureA()->GetBody();
	b2Body* bodyB = contact->GetFixtureB()->GetBody();
	b2Vec2 relativeVelocity = bodyB->GetLinearVelocity() - bodyA->GetLinearVelocity();

	b2Vec2 normal = worldManifold.normal;

	this->other = other;
	this->velocity = glm::vec2{ relativeVelocity.x, relativeVelocity.y };
	this->normal = glm::vec2{ normal.x, normal.y };
	this->fixture = fixture;
	this->otherFixture = otherFixture;
	this->enabled = true;
}
