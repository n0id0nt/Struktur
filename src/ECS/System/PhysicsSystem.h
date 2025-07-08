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
            const float m_pixelsPerPhysicsMeter = 32.f; // pixels per game unit

        public:
            PhysicsSystem()
            {
                b2Vec2 gravity(0.0f, -10.0f);
                m_physicsWorld = std::make_unique<b2World>(gravity);
            }
            
            void Update(Core::GameContext& context) override
            {
                float deltaTime = context.GetGameData().dt;
                stepPhysics(context, deltaTime);
            }

            void stepPhysics(Core::GameContext& context, float deltaTime) 
            {
                syncTransformsToPhysics(context);
                int32 velocityIterations = 8;
                int32 positionIterations = 3;
                m_physicsWorld->Step(1/60.f, velocityIterations, positionIterations);
                
                // Update transforms from physics for dynamic bodies
                syncPhysicsToTransforms(context);
            }

            void syncPhysicsToTransforms(Core::GameContext& context) 
            {
                entt::registry& registry = context.GetRegistry();

                auto view = registry.view<Component::PhysicsBody, Component::Transform>();
                
                for (auto entity : view)
                {
                    auto& physicsBody = view.get<Component::PhysicsBody>(entity);
                    auto& transform = view.get<Component::Transform>(entity);
                    
                    if (physicsBody.body && physicsBody.syncFromPhysics) 
                    {
                        // Get world position from physics
                        b2Vec2 position = physicsBody.body->GetPosition();
                        float angle = physicsBody.body->GetAngle();
                        
                        // Convert to local space if entity has a parent
                        if (auto* parent = registry.try_get<Component::Parent>(entity)) 
                        {
                            if (parent->entity != entt::null) 
                            {
                                glm::vec3 worldPos(position.x * m_pixelsPerPhysicsMeter, position.y * m_pixelsPerPhysicsMeter, 0.0f);
                                glm::vec3 localPos = worldToLocal(context, worldPos, parent->entity);
                                transform.position = localPos;
                                
                                // Handle rotation relative to parent
                                float parentAngle = getWorldRotation(context, parent->entity);
                                float localAngle = angle - parentAngle;
                                transform.rotation = glm::angleAxis(localAngle, glm::vec3(0, 0, 1));
                            }
                            else
                            {
                                // No parent, use world coordinates directly
                                transform.position = glm::vec3(position.x * m_pixelsPerPhysicsMeter, position.y * m_pixelsPerPhysicsMeter, 0.0f);
                                transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                            }
                        }
                        else
                        {
                            // No parent, use world coordinates directly
                            transform.position = glm::vec3(position.x * m_pixelsPerPhysicsMeter, position.y * m_pixelsPerPhysicsMeter, 0.0f);
                            transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                        }
                    }
                }
            }

            void syncTransformsToPhysics(Core::GameContext& context) 
            {
                entt::registry& registry = context.GetRegistry();
                auto view = registry.view<Component::PhysicsBody, Component::Transform, Component::WorldTransform>();
                
                for (auto entity : view) 
                {
                    auto& physicsBody = view.get<Component::PhysicsBody>(entity);
                    auto& worldTransform = view.get<Component::WorldTransform>(entity);
                    
                    if (physicsBody.body && physicsBody.syncToPhysics && physicsBody.isDirty)
                    {
                        // Extract position and rotation from world transform matrix
                        glm::vec3 worldPos = glm::vec3(worldTransform.matrix[3]) / m_pixelsPerPhysicsMeter;
                        
                        // For 2D, extract rotation from matrix
                        float angle = atan2(worldTransform.matrix[1][0], worldTransform.matrix[0][0]);
                        
                        physicsBody.body->SetTransform(b2Vec2(worldPos.x, worldPos.y), angle);
                        physicsBody.isDirty = false;
                    }
                }
            }

            b2Body* createPhysicsBody(Core::GameContext& context, entt::entity entity, const b2BodyDef& bodyDef) 
            {
                entt::registry& registry = context.GetRegistry();
                b2Body* body = m_physicsWorld->CreateBody(&bodyDef);
                body->GetUserData().pointer = static_cast<uintptr_t>(entity);

                //Create Ball shape
                b2PolygonShape groundBox;
                groundBox.SetAsBox(1 / 2.0f, 1 / 2.0f);

                b2FixtureDef fixtureDef;
                fixtureDef.shape = &groundBox;
                fixtureDef.density = 1.f;
                fixtureDef.friction = 0.4;
                fixtureDef.restitution = 0.f; 

                body->CreateFixture(&fixtureDef);
                
                registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);
                
                return body;
            }
            
        private:
            glm::vec3 worldToLocal(Core::GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity) 
            {
                entt::registry& registry = context.GetRegistry();
                if (auto* parentWorld = registry.try_get<Component::WorldTransform>(parentEntity)) 
                {
                    glm::mat4 parentInverse = glm::inverse(parentWorld->matrix);
                    glm::vec4 localPos = parentInverse * glm::vec4(worldPos, 1.0f);
                    return glm::vec3(localPos);
                }
                return worldPos;
            }

            float getWorldRotation(Core::GameContext& context, entt::entity entity) 
            {
                entt::registry& registry = context.GetRegistry();
                if (auto* worldTransform = registry.try_get<Component::WorldTransform>(entity)) 
                {
                    return atan2(worldTransform->matrix[1][0], worldTransform->matrix[0][0]);
                }
                return 0.0f;
            }
        };
    }
}