#pragma once

#include "Engine/Physics/ContactListener.h"
#include "box2d\box2d.h"
#include "glm/glm.hpp"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Physics
{
class PhysicsWorld
{
   public:
	PhysicsWorld();
	void Initialise(glm::vec2 gravity, int velocityIterations, int positionIterations, float pixelsPerMeter);

	// context is only handed to the contact listener for the duration of this call (see ContactListener::
	// SetContext) - BeginContact/EndContact/PreSolve only ever fire synchronously from within b2World::Step().
	void Step(GameContext& context, float deltaTime);

	void ClearForces();

	b2Body* CreateBody(const b2BodyDef* bodyDef);
	void DestroyBody(b2Body* body);

	float GetPixelsPerMeter() const;
	void SetPixelsPerMeter(float pixelsPerMeter);

	b2World* GetRawWorld()
	{
		return &m_world;
	}

	void Clear();

   private:
	ContactListener m_contactListener;

	float m_pixelsPerMeter;

	b2World m_world;

	int m_velocityIteration;
	int m_positionIterations;
};
}  // namespace Physics
}  // namespace Struktur
