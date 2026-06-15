#include "PhysicsWorld.h"

#include "Debug/Assertions.h"

Struktur::Physics::PhysicsWorld::PhysicsWorld()
    : m_world(b2Vec2()),
      m_contactListener()
{
	m_world.SetContactListener(&m_contactListener);
}

void Struktur::Physics::PhysicsWorld::Initialise(glm::vec2 gravity, int velocityIterations, int positionIterations,
                                                 float pixelsPerMeter)
{
	m_world.SetGravity(b2Vec2(gravity.x, gravity.y));
	m_velocityIteration  = velocityIterations;
	m_positionIterations = positionIterations;
	m_pixelsPerMeter     = pixelsPerMeter;
}

void Struktur::Physics::PhysicsWorld::Step(float deltaTime)
{
	m_world.Step(deltaTime, m_velocityIteration, m_positionIterations);
}

void Struktur::Physics::PhysicsWorld::ClearForces()
{
	m_world.ClearForces();
}

b2Body* Struktur::Physics::PhysicsWorld::CreateBody(const b2BodyDef* bodyDef)
{
	return m_world.CreateBody(bodyDef);
}

void Struktur::Physics::PhysicsWorld::DestroyBody(b2Body* body)
{
	m_world.DestroyBody(body);
}

float Struktur::Physics::PhysicsWorld::GetPixelsPerMeter() const
{
	return m_pixelsPerMeter;
}

void Struktur::Physics::PhysicsWorld::SetPixelsPerMeter(float pixelsPerMeter)
{
	m_pixelsPerMeter = pixelsPerMeter;
	// TODO resize everything in the scene to match the new pixel size, or just assert to ensure the scene is empty
	// before resizing - currently only set before any physics objects are created as it is not yet a problem
	BREAK_MSG(
	    "Set Pixels Per Meter should not be called because it would require everything in the physics scene to be "
	    "resized which is not implemented.");
}

void Struktur::Physics::PhysicsWorld::Clear()
{
	b2Body* body = m_world.GetBodyList();
	while (body)
	{
		b2Body* next = body->GetNext();
		m_world.DestroyBody(body);
		body = next;
	}
}
