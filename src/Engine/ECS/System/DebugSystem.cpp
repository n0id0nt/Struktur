#include "DebugSystem.h"

#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"

Struktur::System::DebugSystem::DebugSystem()
    : m_box2dRenderer()
{
}

void Struktur::System::DebugSystem::Update(GameContext& context)
{
	// Get editor settings from context
	auto& debugSettings = context.GetEditor().GetSettings().debugRender;

	entt::registry& registry            = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	GameResource::Camera& camera        = context.GetCamera();

	// Begin 2D mode for world-space rendering
	::BeginMode2D(camera.GetRaylibCamera());

	// Render physics debug visualization (if enabled)
	if (debugSettings.showPhysicsShapes || debugSettings.showPhysicsBodies || debugSettings.showPhysicsJoints ||
	    debugSettings.showPhysicsAABBs)
	{
		m_box2dRenderer.RenderWorld(physicsWorld.GetRawWorld(), physicsWorld.GetPixelsPerMeter(),
		                            debugSettings.showPhysicsShapes,  // shapes
		                            debugSettings.showPhysicsBodies,  // bodies
		                            debugSettings.showPhysicsJoints,  // joints
		                            debugSettings.showPhysicsAABBs    // AABBs
		);
	}

	// Render physics contact points (if enabled)
	if (debugSettings.showPhysicsContactPoints)
	{
		// TODO: Implement contact point rendering
		// m_box2dRenderer.RenderContactPoints(...);
	}

	// Render level boundaries (if enabled)
	if (debugSettings.showLevelBounds)
	{
		TransformSystem& transformSystem = context.GetSystemManager().GetSystem<TransformSystem>();
		auto view = registry.view<Component::Level, Component::Transform>(entt::exclude<Inactive>);
		for (auto [entity, level, transform] : view.each())
		{
			glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);
			::Rectangle levelBounds{worldPosition.x, worldPosition.y, (float)level.width, (float)level.height};
			::DrawRectangleLinesEx(levelBounds, debugSettings.levelBoundsThickness, ORANGE);
		}
	}

	// Render entity gizmos (if enabled)
	if (debugSettings.showEntityGizmos)
	{
		RenderEntityGizmos(context);
	}

	// Render grid (if enabled)
	if (debugSettings.showGrid)
	{
		RenderGrid(context);
	}

	::EndMode2D();

	// Render FPS counter (if enabled) - screen space
	if (debugSettings.showFPS)
	{
		::DrawFPS(10, 10);
	}
}

void Struktur::System::DebugSystem::RenderEntityGizmos(GameContext& context)
{
	entt::registry& registry         = context.GetRegistry();
	TransformSystem& transformSystem = context.GetSystemManager().GetSystem<TransformSystem>();

	// Render position indicators for all entities with transforms
	auto view = registry.view<Component::Transform>(entt::exclude<Inactive>);
	for (auto [entity, transform] : view.each())
	{
		glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);

		// Draw a small cross at entity position
		float crossSize = 10.0f;
		::Vector2 pos   = {worldPosition.x, worldPosition.y};

		::DrawLineEx({pos.x - crossSize, pos.y}, {pos.x + crossSize, pos.y}, 2.0f, GREEN);
		::DrawLineEx({pos.x, pos.y - crossSize}, {pos.x, pos.y + crossSize}, 2.0f, GREEN);
	}
}

void Struktur::System::DebugSystem::RenderGrid(GameContext& context)
{
	auto& gridSettings           = context.GetEditor().GetSettings().grid;
	GameResource::Camera& camera = context.GetCamera();
	Core::GameData& gameData     = context.GetGameData();

	// Calculate visible world bounds from camera
	// Get screen corners and convert to world space
	glm::vec2 topLeft     = camera.ScreenPosToWorldPos(glm::vec2(0, 0));
	glm::vec2 bottomRight = camera.ScreenPosToWorldPos(glm::vec2(gameData.gameWidth, gameData.gameHeight));

	float gridSize      = gridSettings.gridSize;
	unsigned char alpha = (unsigned char)(gridSettings.gridOpacity * 255.0f);
	::Color gridColor   = {200, 200, 200, alpha};

	// Calculate grid start/end positions (expand slightly for rotation)
	float margin = gridSize * 2;
	int startX   = (int)((topLeft.x - margin) / gridSize) * gridSize;
	int endX     = (int)((bottomRight.x + margin) / gridSize + 1) * gridSize;
	int startY   = (int)((topLeft.y - margin) / gridSize) * gridSize;
	int endY     = (int)((bottomRight.y + margin) / gridSize + 1) * gridSize;

	// Draw vertical lines
	for (int x = startX; x <= endX; x += gridSize)
	{
		::DrawLine(x, startY, x, endY, gridColor);
	}

	// Draw horizontal lines
	for (int y = startY; y <= endY; y += gridSize)
	{
		::DrawLine(startX, y, endX, y, gridColor);
	}

	// Draw origin lines (thicker and colored)
	if (startX <= 0 && endX >= 0)
	{
		::DrawLineEx({0, (float)startY}, {0, (float)endY}, 2.0f, RED);
	}
	if (startY <= 0 && endY >= 0)
	{
		::DrawLineEx({(float)startX, 0}, {(float)endX, 0}, 2.0f, GREEN);
	}
}
