#include "DebugSystem.h"

#include <format>

#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"

Struktur::System::DebugSystem::DebugSystem()
{
}

void Struktur::System::DebugSystem::Update(GameContext& context)
{
	auto& debugSettings                 = context.GetEditor().GetSettings().debugRender;
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	m_debugRenderer.SetupView(context);

	// box2d's own b2Draw walk (see b2World::DebugDraw) replaces Box2DDebugRenderer's manual raylib-coupled walk
	// on this path - its flags map onto real box2d concepts rather than Box2DDebugRenderer::RenderWorld's
	// positional-argument reuse (e.g. web's "showPhysicsBodies" setting happens to control that function's
	// drawCenterOfMass argument only because of argument order, not because the names actually correspond).
	// Shapes and center-of-mass markers (e_shapeBit/e_centerOfMassBit) are deliberately NOT included in this
	// walk - box2d's own e_centerOfMassBit loop has no IsEnabled() check at all (unlike its e_aabbBit loop),
	// so it would draw every disabled body's axis marker too - see RenderPhysicsShapes below instead.
	if (debugSettings.showPhysicsJoints || debugSettings.showPhysicsAABBs)
	{
		uint32 flags = 0;
		if (debugSettings.showPhysicsJoints)
		{
			flags |= b2Draw::e_jointBit;
		}
		if (debugSettings.showPhysicsAABBs)
		{
			flags |= b2Draw::e_aabbBit;
		}

		m_box2dBgfxDebugDraw.SetPixelsPerMeter(physicsWorld.GetPixelsPerMeter());
		m_box2dBgfxDebugDraw.SetFlags(flags);
		physicsWorld.GetRawWorld()->SetDebugDraw(&m_box2dBgfxDebugDraw);
		physicsWorld.GetRawWorld()->DebugDraw();
	}
	if (debugSettings.showPhysicsShapes || debugSettings.showPhysicsBodies)
	{
		RenderPhysicsShapes(context);
	}
	// showPhysicsContactPoints: dead on web too (// TODO: Implement contact point rendering, never called) - not ported.

	if (debugSettings.showLevelBounds)
	{
		RenderLevelBounds(context);
	}
	if (debugSettings.showEntityGizmos)
	{
		RenderEntityGizmos(context);
	}
	if (debugSettings.showGrid)
	{
		RenderGrid(context);
	}

	if (debugSettings.showFPS)
	{
		// Screen-space text is UIRenderer's domain (UIViewId), not the world-space DebugRenderer's - reuses the
		// same already-pooled "default" font UILabel uses. Fetched once and cached on m_fpsFont instead of
		// calling GetFont() every frame - still a cache hit (not a fresh disk load) either way, but this skips
		// the ResourcePool map lookup and refcount churn on every single frame just to draw a counter.
		if (!m_fpsFont)
		{
			m_fpsFont = context.GetResourceManager().GetFont(context, "default", 32);
		}
		if (!m_fpsFont->IsGpuReady())
		{
			m_fpsFont->LoadToGpu(context);
		}
		float fps = context.GetTimeSystem().unscaledDelta > 0.0f ? 1.0f / context.GetTimeSystem().unscaledDelta : 0.0f;
		context.GetUIRenderer().DrawText(m_fpsFont->font, std::format("FPS: {:.0f}", fps), {10, 10}, 16.0f,
		                                 Util::ColorGreen);
	}
}

void Struktur::System::DebugSystem::RenderEntityGizmos(GameContext& context)
{
	entt::registry& registry         = context.GetRegistry();
	TransformSystem& transformSystem = context.GetSystemManager().GetSystem<TransformSystem>();

	auto view = registry.view<Component::Transform>(entt::exclude<Inactive>);
	for (auto [entity, transform] : view.each())
	{
		glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);

		float crossSize = 10.0f;
		glm::vec2 pos    = {worldPosition.x, worldPosition.y};

		m_debugRenderer.DrawLine({pos.x - crossSize, pos.y}, {pos.x + crossSize, pos.y}, 2.0f, Util::ColorGreen);
		m_debugRenderer.DrawLine({pos.x, pos.y - crossSize}, {pos.x, pos.y + crossSize}, 2.0f, Util::ColorGreen);
	}
}

void Struktur::System::DebugSystem::RenderPhysicsShapes(GameContext& context)
{
	auto& debugSettings                 = context.GetEditor().GetSettings().debugRender;
	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

	// box2d's own b2World::DebugDraw walk has no concept of Component::Active - a disabled body just gets a
	// dimmer tint (see the IsEnabled() branch below), it's never skipped. Walking bodies through this entt view
	// instead (mirroring RenderEntityGizmos/RenderLevelBounds's Inactive exclusion) means an inactive entity's
	// collision shapes don't render at all, matching how showEntityGizmos/showLevelBounds already behave.
	m_box2dBgfxDebugDraw.SetPixelsPerMeter(physicsWorld.GetPixelsPerMeter());

	auto view = registry.view<Component::PhysicsBody>(entt::exclude<Inactive>);
	for (auto [entity, physicsBody] : view.each())
	{
		b2Body* body = physicsBody.body;
		if (!body)
		{
			continue;
		}

		b2Color color;
		if (body->GetType() == b2_dynamicBody && body->GetMass() == 0.0f)
		{
			color = b2Color(1.0f, 0.0f, 0.0f);
		}
		else if (!body->IsEnabled())
		{
			color = b2Color(0.5f, 0.5f, 0.3f);
		}
		else if (body->GetType() == b2_staticBody)
		{
			color = b2Color(0.5f, 0.9f, 0.5f);
		}
		else if (body->GetType() == b2_kinematicBody)
		{
			color = b2Color(0.5f, 0.5f, 0.9f);
		}
		else if (!body->IsAwake())
		{
			color = b2Color(0.6f, 0.6f, 0.6f);
		}
		else
		{
			color = b2Color(0.9f, 0.7f, 0.7f);
		}

		const b2Transform& xf = body->GetTransform();
		if (debugSettings.showPhysicsShapes)
		{
			for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
			{
				switch (fixture->GetType())
				{
					case b2Shape::e_circle:
					{
						b2CircleShape* circle = (b2CircleShape*)fixture->GetShape();
						b2Vec2 center         = b2Mul(xf, circle->m_p);
						float radius          = circle->m_radius;
						b2Vec2 axis           = b2Mul(xf.q, b2Vec2(1.0f, 0.0f));
						m_box2dBgfxDebugDraw.DrawSolidCircle(center, radius, axis, color);
						break;
					}
					case b2Shape::e_edge:
					{
						b2EdgeShape* edge = (b2EdgeShape*)fixture->GetShape();
						b2Vec2 v1          = b2Mul(xf, edge->m_vertex1);
						b2Vec2 v2          = b2Mul(xf, edge->m_vertex2);
						m_box2dBgfxDebugDraw.DrawSegment(v1, v2, color);
						break;
					}
					case b2Shape::e_chain:
					{
						// Tile map collision (see TileMapCollisionBodyGenerator) is built as a single chain
						// shape per body - box2d's own DrawShape draws this as a segment strip, not a closed
						// loop (matches CreateLoop's vertex list, which already omits the duplicate final
						// vertex before calling b2ChainShape::CreateLoop).
						b2ChainShape* chain    = (b2ChainShape*)fixture->GetShape();
						int32 count             = chain->m_count;
						const b2Vec2* vertices = chain->m_vertices;
						b2Vec2 v1               = b2Mul(xf, vertices[0]);
						for (int32 i = 1; i < count; ++i)
						{
							b2Vec2 v2 = b2Mul(xf, vertices[i]);
							m_box2dBgfxDebugDraw.DrawSegment(v1, v2, color);
							v1 = v2;
						}
						break;
					}
					case b2Shape::e_polygon:
					{
						b2PolygonShape* poly = (b2PolygonShape*)fixture->GetShape();
						int32 vertexCount     = poly->m_count;
						b2Vec2 vertices[b2_maxPolygonVertices];
						for (int32 i = 0; i < vertexCount; ++i)
						{
							vertices[i] = b2Mul(xf, poly->m_vertices[i]);
						}
						m_box2dBgfxDebugDraw.DrawSolidPolygon(vertices, vertexCount, color);
						break;
					}
					default:
						break;
				}
			}
		}

		if (debugSettings.showPhysicsBodies)
		{
			// Mirrors box2d's own e_centerOfMassBit walk (see b2World::DebugDraw), but per-entity so an
			// inactive/disabled body's axis marker is skipped along with its shapes - box2d's built-in walk
			// draws this marker for every body unconditionally, with no IsEnabled() check at all.
			b2Transform centerXf = xf;
			centerXf.p            = body->GetWorldCenter();
			m_box2dBgfxDebugDraw.DrawTransform(centerXf);
		}
	}
}

void Struktur::System::DebugSystem::RenderGrid(GameContext& context)
{
	auto& gridSettings            = context.GetEditor().GetSettings().grid;
	World::Camera& camera = context.GetCamera();
	Core::GameData& gameData     = context.GetGameData();

	glm::vec2 topLeft     = camera.ScreenPosToWorldPos(glm::vec2(0, 0));
	glm::vec2 bottomRight = camera.ScreenPosToWorldPos(glm::vec2(gameData.gameWidth, gameData.gameHeight));

	float gridSize      = gridSettings.gridSize;
	unsigned char alpha = (unsigned char)(gridSettings.gridOpacity * 255.0f);
	Util::Color gridColor = {200, 200, 200, alpha};

	float margin = gridSize * 2;
	int startX    = (int)((topLeft.x - margin) / gridSize) * gridSize;
	int endX      = (int)((bottomRight.x + margin) / gridSize + 1) * gridSize;
	int startY    = (int)((topLeft.y - margin) / gridSize) * gridSize;
	int endY      = (int)((bottomRight.y + margin) / gridSize + 1) * gridSize;

	for (int x = startX; x <= endX; x += gridSize)
	{
		m_debugRenderer.DrawLine({(float)x, (float)startY}, {(float)x, (float)endY}, 1.0f, gridColor);
	}
	for (int y = startY; y <= endY; y += gridSize)
	{
		m_debugRenderer.DrawLine({(float)startX, (float)y}, {(float)endX, (float)y}, 1.0f, gridColor);
	}

	if (startX <= 0 && endX >= 0)
	{
		m_debugRenderer.DrawLine({0.0f, (float)startY}, {0.0f, (float)endY}, 2.0f, Util::ColorRed);
	}
	if (startY <= 0 && endY >= 0)
	{
		m_debugRenderer.DrawLine({(float)startX, 0.0f}, {(float)endX, 0.0f}, 2.0f, Util::ColorGreen);
	}
}

void Struktur::System::DebugSystem::RenderLevelBounds(GameContext& context)
{
	auto& debugSettings               = context.GetEditor().GetSettings().debugRender;
	entt::registry& registry          = context.GetRegistry();
	TransformSystem& transformSystem  = context.GetSystemManager().GetSystem<TransformSystem>();

	auto view = registry.view<Component::Level, Component::Transform>(entt::exclude<Inactive>);
	for (auto [entity, level, transform] : view.each())
	{
		glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);
		glm::vec2 min            = {worldPosition.x, worldPosition.y};
		glm::vec2 max            = {worldPosition.x + level.width, worldPosition.y + level.height};
		m_debugRenderer.DrawRectOutline(min, max, debugSettings.levelBoundsThickness, Util::ColorOrange);
	}
}
