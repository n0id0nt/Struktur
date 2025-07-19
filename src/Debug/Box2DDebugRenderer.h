#pragma once 

#include <vector>
#include "raylib.h"
#include "box2d/box2d.h"

namespace Struktur
{
    namespace Debug
    {
        class Box2DDebugRenderer
        {
        private:
            Color m_lineColor;
            Color m_fillColor;
            float m_lineThickness;

            // Helper function to convert Box2D coordinates to screen coordinates
            Vector2 B2ToScreen(const b2Vec2& b2Pos, float scale)
            {
                return { b2Pos.x * scale, b2Pos.y * scale };
            }

            // Draw a polygon shape
            void DrawPolygonShape(const b2PolygonShape* polygon, const b2Transform& transform, float scale, bool filled)
            {
                int vertexCount = polygon->m_count;
                if (vertexCount < 3) return;

                std::vector<Vector2> screenVertices;

                // Transform vertices to world coordinates, then to screen coordinates
                for (int i = 0; i < vertexCount; i++)
                {
                    b2Vec2 worldVertex = ::b2Mul(transform, polygon->m_vertices[i]);
                    screenVertices.push_back(B2ToScreen(worldVertex, scale));
                }

                // Draw filled polygon if requested
                if (filled)
                {
                    for (int i = 1; i < vertexCount - 1; i++)
                    {
                        ::DrawTriangle(screenVertices[0], screenVertices[i], screenVertices[i + 1], m_fillColor);
                    }
                }

                // Draw polygon outline
                for (int i = 0; i < vertexCount; i++)
                {
                    int next = (i + 1) % vertexCount;
                    ::DrawLineEx(screenVertices[i], screenVertices[next], m_lineThickness, m_lineColor);
                }
            }

            // Draw a circle shape
            void DrawCircleShape(const b2CircleShape* circle, const b2Transform& transform, float scale, bool filled)
            {
                ::b2Vec2 worldCenter = ::b2Mul(transform, circle->m_p);
                ::Vector2 screenCenter = B2ToScreen(worldCenter, scale);
                float screenRadius = circle->m_radius * scale;

                // Draw filled circle if requested
                if (filled)
                {
                    ::DrawCircleV(screenCenter, screenRadius, m_fillColor);
                }

                // Draw circle outline
                ::DrawCircleLines(screenCenter.x, screenCenter.y, screenRadius, m_lineColor);

                // Draw radius line to show orientation
                ::b2Vec2 radiusDirection = ::b2Mul(transform.q, ::b2Vec2(circle->m_radius, 0.0f));
                ::Vector2 radiusEnd = {
                    screenCenter.x + radiusDirection.x * scale,
                    screenCenter.y + radiusDirection.y * scale
                };
                ::DrawLineEx(screenCenter, radiusEnd, m_lineThickness * 0.5f, m_lineColor);
            }

            // Draw an edge shape (line segment)
            void DrawEdgeShape(const ::b2EdgeShape* edge, const ::b2Transform& transform, float scale)
            {
                ::b2Vec2 worldVertex1 = ::b2Mul(transform, edge->m_vertex1);
                ::b2Vec2 worldVertex2 = ::b2Mul(transform, edge->m_vertex2);

                ::Vector2 screenVertex1 = B2ToScreen(worldVertex1, scale);
                ::Vector2 screenVertex2 = B2ToScreen(worldVertex2, scale);

                ::DrawLineEx(screenVertex1, screenVertex2, m_lineThickness, m_lineColor);
            }

            // Draw a chain shape
            void DrawChainShape(const ::b2ChainShape* chain, const ::b2Transform& transform, float scale)
            {
                for (int i = 0; i < chain->m_count - 1; i++)
                {
                    ::b2Vec2 worldVertex1 = ::b2Mul(transform, chain->m_vertices[i]);
                    ::b2Vec2 worldVertex2 = ::b2Mul(transform, chain->m_vertices[i + 1]);

                    ::Vector2 screenVertex1 = B2ToScreen(worldVertex1, scale);
                    ::Vector2 screenVertex2 = B2ToScreen(worldVertex2, scale);

                    DrawLineEx(screenVertex1, screenVertex2, m_lineThickness, m_lineColor);
                }
            }

            // Draw velocity arrow from body center
            void DrawVelocityArrow(::b2Body* body, ::Color velocityColor, float velocityScale, float scale)
            {
                if (!body) return;

                ::b2Vec2 velocity = body->GetLinearVelocity();
                float velocityMagnitude = velocity.Length();

                // Don't draw very small velocities to avoid clutter
                if (velocityMagnitude < 0.1f) return;

                ::Vector2 bodyCenter = B2ToScreen(body->GetWorldCenter(), scale);

                // Scale velocity for visualization
                ::b2Vec2 scaledVelocity = velocityScale * scale * velocity;
                ::Vector2 velocityEnd = {
                    bodyCenter.x + scaledVelocity.x,
                    bodyCenter.y + scaledVelocity.y
                };

                // Draw velocity line
                ::DrawLineEx(bodyCenter, velocityEnd, m_lineThickness * 1.5f, velocityColor);

                // Draw arrowhead if velocity is significant
                if (velocityMagnitude > 0.5f)
                {
                    // Calculate arrowhead points
                    float arrowLength = 8.0f;
                    float arrowAngle = 0.5f; // radians

                    ::b2Vec2 velocityDirection = velocity;
                    velocityDirection.Normalize();

                    // Arrow points
                    float cos_angle = ::cosf(arrowAngle);
                    float sin_angle = ::sinf(arrowAngle);

                    ::Vector2 arrowPoint1 = {
                        velocityEnd.x - arrowLength * (velocityDirection.x * cos_angle - velocityDirection.y * sin_angle),
                        velocityEnd.y - arrowLength * (velocityDirection.x * sin_angle + velocityDirection.y * cos_angle)
                    };

                    ::Vector2 arrowPoint2 = {
                        velocityEnd.x - arrowLength * (velocityDirection.x * cos_angle + velocityDirection.y * sin_angle),
                        velocityEnd.y - arrowLength * (-velocityDirection.x * sin_angle + velocityDirection.y * cos_angle)
                    };

                    // Draw arrowhead
                    ::DrawTriangle(velocityEnd, arrowPoint1, arrowPoint2, velocityColor);
                }
            }

            // Draw angular velocity indicator (circular arrow around body)
            void DrawAngularVelocity(::b2Body* body, ::Color angularColor, float radiusScale, float scale)
            {
                if (!body) return;

                float angularVelocity = body->GetAngularVelocity();

                // Don't draw very small angular velocities
                if (::fabsf(angularVelocity) < 0.1f) return;

                ::Vector2 bodyCenter = B2ToScreen(body->GetWorldCenter(), scale);

                // Calculate radius based on body size (approximate)
                float radius = radiusScale * scale;

                // Draw partial circle to show angular velocity direction and magnitude
                float arcLength = ::fminf(::fabsf(angularVelocity) * 0.5f, PI); // Limit arc length
                int segments = (int)(arcLength * 20); // More segments for smoother arc
                segments = ::fmaxf(segments, 8); // Minimum segments

                float startAngle = 0.0f;
                float angleStep = arcLength / segments;

                if (angularVelocity < 0) angleStep = -angleStep; // Clockwise

                for (int i = 0; i < segments; i++)
                {
                    float angle1 = startAngle + i * angleStep;
                    float angle2 = startAngle + (i + 1) * angleStep;

                    ::Vector2 point1 = {
                        bodyCenter.x + ::cosf(angle1) * radius,
                        bodyCenter.y + ::sinf(angle1) * radius
                    };
                    ::Vector2 point2 = {
                        bodyCenter.x + ::cosf(angle2) * radius,
                        bodyCenter.y + ::sinf(angle2) * radius
                    };

                    ::DrawLineEx(point1, point2, m_lineThickness, angularColor);
                }

                // Draw small arrow at the end to show direction
                float finalAngle = startAngle + segments * angleStep;
                ::Vector2 arcEnd = {
                    bodyCenter.x + ::cosf(finalAngle) * radius,
                    bodyCenter.y + ::sinf(finalAngle) * radius
                };

                // Small arrow perpendicular to radius
                float arrowSize = 5.0f;
                ::Vector2 arrowDir = { -::sinf(finalAngle), ::cosf(finalAngle) };
                if (angularVelocity < 0) { arrowDir.x = -arrowDir.x; arrowDir.y = -arrowDir.y; }

                ::Vector2 arrowTip = {
                    arcEnd.x + arrowDir.x * arrowSize,
                    arcEnd.y + arrowDir.y * arrowSize
                };

                ::DrawLineEx(arcEnd, arrowTip, m_lineThickness, angularColor);
            }

        public:
            Box2DDebugRenderer(::Color line = GREEN, ::Color fill = { 0, 255, 0, 50 }, float thickness = 1.0f)
                : m_lineColor(line), m_fillColor(fill), m_lineThickness(thickness)
            {
            }

            // Main function to render all physics bodies in the world
            void RenderWorld(::b2World* world, float scale = 32.0f, bool drawFilled = false, bool drawCenterOfMass = false, bool drawVelocity = false, bool drawAngularVelocity = false)
            {
                // Iterate through all bodies in the world
                for (::b2Body* body = world->GetBodyList(); body != nullptr; body = body->GetNext())
                {
                    RenderBody(body, scale, drawFilled, drawCenterOfMass, drawVelocity, drawAngularVelocity);
                }
            }

            // Render a specific physics body
            void RenderBody(::b2Body* body, float scale = 32.0f, bool drawFilled = false, bool drawCenterOfMass = false, bool drawVelocity = false, bool drawAngularVelocity = false)
            {
                if (!body) return;

                ::b2Transform transform = body->GetTransform();

                // Iterate through all fixtures of this body
                for (::b2Fixture* fixture = body->GetFixtureList(); fixture != nullptr; fixture = fixture->GetNext())
                {
                    RenderFixture(fixture, transform, scale, drawFilled);
                }

                // Draw center of mass if requested
                if (drawCenterOfMass)
                {
                    ::Vector2 centerOfMass = B2ToScreen(body->GetWorldCenter(), scale);
                    DrawCircleV(centerOfMass, 3.0f, RED);
                    DrawCircleLines(centerOfMass.x, centerOfMass.y, 3.0f, DARKPURPLE);
                }

                // Draw velocity arrow if requested
                if (drawVelocity)
                {
                    DrawVelocityArrow(body, YELLOW, 0.1f, scale);
                }

                // Draw angular velocity if requested
                if (drawAngularVelocity)
                {
                    DrawAngularVelocity(body, ORANGE, 1.5f, scale);
                }
            }

            // Render a specific fixture
            void RenderFixture(::b2Fixture* fixture, const ::b2Transform& transform, float scale = 32.0f, bool drawFilled = false)
            {
                if (!fixture) return;

                const b2Shape* shape = fixture->GetShape();
                b2Shape::Type shapeType = shape->GetType();

                switch (shapeType)
                {
                case b2Shape::e_polygon:
                {
                    const b2PolygonShape* polygon = static_cast<const b2PolygonShape*>(shape);
                    DrawPolygonShape(polygon, transform, scale, drawFilled);
                    break;
                }
                case b2Shape::e_circle:
                {
                    const b2CircleShape* circle = static_cast<const b2CircleShape*>(shape);
                    DrawCircleShape(circle, transform, scale, drawFilled);
                    break;
                }
                case b2Shape::e_edge:
                {
                    const b2EdgeShape* edge = static_cast<const b2EdgeShape*>(shape);
                    DrawEdgeShape(edge, transform, scale);
                    break;
                }
                case b2Shape::e_chain:
                {
                    const b2ChainShape* chain = static_cast<const b2ChainShape*>(shape);
                    DrawChainShape(chain, transform, scale);
                    break;
                }
                default:
                    break;
                }
            }

            // Render only specific bodies that match a condition
            template<typename Predicate>
            void RenderBodiesIf(b2World& world, Predicate pred, float scale, bool drawFilled, bool drawCenterOfMass, bool drawVelocity, bool drawAngularVelocity)
            {
                for (b2Body* body = world.GetBodyList(); body != nullptr; body = body->GetNext())
                {
                    if (pred(body))
                    {
                        RenderBody(body, scale, drawFilled, drawCenterOfMass, drawVelocity, drawAngularVelocity);
                    }
                }
            }

            // Render only dynamic bodies
            void RenderDynamicBodies(b2World& world, float scale, bool drawFilled, bool drawCenterOfMass, bool drawVelocity, bool drawAngularVelocity)
            {
                RenderBodiesIf(world, [](b2Body* body) {
                    return body->GetType() == b2_dynamicBody;
                    }, scale, drawFilled, drawCenterOfMass, drawVelocity, drawAngularVelocity);
            }

            // Render only static bodies
            void RenderStaticBodies(b2World& world, float scale, bool drawFilled, bool drawCenterOfMass, bool drawVelocity, bool drawAngularVelocity)
            {
                RenderBodiesIf(world, [](b2Body* body) {
                    return body->GetType() == b2_staticBody;
                    }, scale, drawFilled, drawCenterOfMass, drawVelocity, drawAngularVelocity);
            }

            // Render only kinematic bodies
            void RenderKinematicBodies(b2World& world, float scale, bool drawFilled, bool drawCenterOfMass, bool drawVelocity, bool drawAngularVelocity)
            {
                RenderBodiesIf(world, [](b2Body* body) {
                    return body->GetType() == b2_kinematicBody;
                    }, scale, drawFilled, drawCenterOfMass, drawVelocity, drawAngularVelocity);
            }

            // Utility functions to change appearance
            void SetLineColor(Color color) { m_lineColor = color; }
            void SetFillColor(Color color) { m_fillColor = color; }
            void SetLineThickness(float thickness) { m_lineThickness = thickness; }

            // Get current settings
            Color GetLineColor() const { return m_lineColor; }
            Color GetFillColor() const { return m_fillColor; }
            float GetLineThickness() const { return m_lineThickness; }
        };
    }
}