#pragma once

#include "box2d\box2d.h"
#include "glm/glm.hpp"

#include "Engine/Physics/ContactListener.h"

namespace Struktur
{
	namespace Physics
	{
		class PhysicsWorld
		{
		public:
			PhysicsWorld(glm::vec2 gravity, int velocityIterations, int positionIterations, float pixelsPerMeter);

			void Step(float deltaTime);

			void ClearForces();

			b2Body* CreateBody(const b2BodyDef* bodyDef);
			void DestroyBody(b2Body* body);

			float GetPixelsPerMeter() const;
			void SetPixelsPerMeter(float pixelsPerMeter);

			b2World* GetRawWorld() { return &m_world; }

			void Clear();

		private:

			ContactListener m_contactListener;

			float m_pixelsPerMeter;

			b2World m_world;

			int m_velocityIteration;
			int m_positionIterations;
		};
	}
}
