#include "DebugSystem.h"

#include <format>

#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/ResourcePtr.h"

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
	if (debugSettings.showPhysicsShapes || debugSettings.showPhysicsBodies || debugSettings.showPhysicsJoints ||
	    debugSettings.showPhysicsAABBs)
	{
		uint32 flags = 0;
		if (debugSettings.showPhysicsShapes)
		{
			flags |= b2Draw::e_shapeBit;
		}
		if (debugSettings.showPhysicsBodies)
		{
			flags |= b2Draw::e_centerOfMassBit;
		}
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
		                                 Util::Math::ColorGreen);
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

		m_debugRenderer.DrawLine({pos.x - crossSize, pos.y}, {pos.x + crossSize, pos.y}, 2.0f, Util::Math::ColorGreen);
		m_debugRenderer.DrawLine({pos.x, pos.y - crossSize}, {pos.x, pos.y + crossSize}, 2.0f, Util::Math::ColorGreen);
	}
}

void Struktur::System::DebugSystem::RenderGrid(GameContext& context)
{
	auto& gridSettings            = context.GetEditor().GetSettings().grid;
	GameResource::Camera& camera = context.GetCamera();
	Core::GameData& gameData     = context.GetGameData();

	glm::vec2 topLeft     = camera.ScreenPosToWorldPos(glm::vec2(0, 0));
	glm::vec2 bottomRight = camera.ScreenPosToWorldPos(glm::vec2(gameData.gameWidth, gameData.gameHeight));

	float gridSize      = gridSettings.gridSize;
	unsigned char alpha = (unsigned char)(gridSettings.gridOpacity * 255.0f);
	Util::Math::Color gridColor = {200, 200, 200, alpha};

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
		m_debugRenderer.DrawLine({0.0f, (float)startY}, {0.0f, (float)endY}, 2.0f, Util::Math::ColorRed);
	}
	if (startY <= 0 && endY >= 0)
	{
		m_debugRenderer.DrawLine({(float)startX, 0.0f}, {(float)endX, 0.0f}, 2.0f, Util::Math::ColorGreen);
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
		m_debugRenderer.DrawRectOutline(min, max, debugSettings.levelBoundsThickness, Util::Math::ColorOrange);
	}
}
