#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"
#include "box2d/box2d.h"

#include "ECS/Component/Transform.h"
#include "ECS/Component/PhysicsBody.h"
#include "Engine/Core/GameContext.h"

namespace Struktur
{
	namespace System
	{
        class PhysicsSystem : public Core::ISystem
        {
        private:
            std::unique_ptr<b2World> m_physicsWorld;

        public:
            PhysicsSystem()
            {
                b2Vec2 gravity(0.0f, -10.0f);
                m_physicsWorld = std::make_unique<b2World>(gravity);
            }
            
            void Update(Struktur::Core::GameContext& context) override
            {
                float deltaTime = context.GetGameData().dt;
                stepPhysics(context, deltaTime);
            }

            void stepPhysics(Struktur::Core::GameContext& context, float deltaTime) 
            {
                syncTransformsToPhysics(context);

                m_physicsWorld->Step(deltaTime, 6, 2);
                
                // Update transforms from physics for dynamic bodies
                syncPhysicsToTransforms(context);
            }

            void syncPhysicsToTransforms(Struktur::Core::GameContext& context) 
            {
                entt::registry& registry = context.GetRegistry();

                auto view = registry.view<Component::PhysicsBody, Struktur::Component::Transform>();
                
                for (auto entity : view) 
                {
                    auto& physicsBody = view.get<Component::PhysicsBody>(entity);
                    auto& transform = view.get<Struktur::Component::Transform>(entity);
                    
                    if (physicsBody.body && physicsBody.syncFromPhysics) 
                    {
                        // Get world position from physics
                        b2Vec2 position = physicsBody.body->GetPosition();
                        float angle = physicsBody.body->GetAngle();
                        
                        // Convert to local space if entity has a parent
                        if (auto* parent = registry.try_get<Struktur::Component::Parent>(entity)) 
                        {
                            if (parent->entity != entt::null) {
                                glm::vec3 worldPos(position.x, position.y, 0.0f);
                                glm::vec3 localPos = worldToLocal(context, worldPos, parent->entity);
                                transform.position = localPos;
                                
                                // Handle rotation relative to parent
                                float parentAngle = getWorldRotation(context, parent->entity);
                                float localAngle = angle - parentAngle;
                                transform.rotation = glm::angleAxis(localAngle, glm::vec3(0, 0, 1));
                            } else {
                                // No parent, use world coordinates directly
                                transform.position = glm::vec3(position.x, position.y, 0.0f);
                                transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                            }
                        } 
                        else 
                        {
                            // No parent, use world coordinates directly
                            transform.position = glm::vec3(position.x, position.y, 0.0f);
                            transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                        }
                    }
                }
            }

            void syncTransformsToPhysics(Struktur::Core::GameContext& context) 
            {
                entt::registry& registry = context.GetRegistry();
                auto view = registry.view<Component::PhysicsBody, Struktur::Component::Transform, Struktur::Component::WorldTransform>();
                
                for (auto entity : view) 
                {
                    auto& physicsBody = view.get<Component::PhysicsBody>(entity);
                    auto& worldTransform = view.get<Struktur::Component::WorldTransform>(entity);
                    
                    if (physicsBody.body && physicsBody.syncToPhysics) 
                    {
                        // Extract position and rotation from world transform matrix
                        glm::vec3 worldPos = glm::vec3(worldTransform.matrix[3]);
                        
                        // For 2D, extract rotation from matrix
                        float angle = atan2(worldTransform.matrix[1][0], worldTransform.matrix[0][0]);
                        
                        physicsBody.body->SetTransform(b2Vec2(worldPos.x, worldPos.y), angle);
                    }
                }
            }

            b2Body* createPhysicsBody(Struktur::Core::GameContext& context, entt::entity entity, const b2BodyDef& bodyDef) 
            {
                entt::registry& registry = context.GetRegistry();
                b2Body* body = m_physicsWorld->CreateBody(&bodyDef);
                body->GetUserData().pointer = static_cast<uintptr_t>(entity);

                //Create Ball shape
                b2CircleShape circleBox;
                circleBox.m_radius = 16.f;

                b2FixtureDef fixtureDef;
                fixtureDef.shape = &circleBox;
                fixtureDef.density = 1.f;
                fixtureDef.friction = 0.4;
                fixtureDef.restitution = 0.f; 

                body->CreateFixture(&fixtureDef);
                
                registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);
                
                return body;
            }
            
        private:
            glm::vec3 worldToLocal(Struktur::Core::GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity) 
            {
                entt::registry& registry = context.GetRegistry();
                if (auto* parentWorld = registry.try_get<Struktur::Component::WorldTransform>(parentEntity)) 
                {
                    glm::mat4 parentInverse = glm::inverse(parentWorld->matrix);
                    glm::vec4 localPos = parentInverse * glm::vec4(worldPos, 1.0f);
                    return glm::vec3(localPos);
                }
                return worldPos;
            }

            float getWorldRotation(Struktur::Core::GameContext& context, entt::entity entity) 
            {
                entt::registry& registry = context.GetRegistry();
                if (auto* worldTransform = registry.try_get<Struktur::Component::WorldTransform>(entity)) 
                {
                    return atan2(worldTransform->matrix[1][0], worldTransform->matrix[0][0]);
                }
                return 0.0f;
            }
        };
    }
}