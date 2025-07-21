#pragma once

#include <vector>
#include "box2d/box2d.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

namespace Struktur
{
    namespace Physics
    {
        class ContactListener : public b2ContactListener {
        public:
            struct Contact {
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

            void BeginContact(b2Contact* contact) override;
            void EndContact(b2Contact* contact) override;

            void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

        private:
            void GetContactEntities(b2Contact* contact, entt::entity entityA, entt::entity entityB);

        };
    }
}
