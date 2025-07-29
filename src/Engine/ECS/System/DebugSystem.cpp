#include "DebugSystem.h"

#include "raylib.h"

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Level.h"

Struktur::System::DebugSystem::DebugSystem() : m_box2dRenderer()
{
}

void Struktur::System::DebugSystem::Update(GameContext &context)
{
	entt::registry& registry = context.GetRegistry();
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	GameResource::Camera& camera = context.GetCamera();

	::BeginMode2D(camera.GetRaylibCamera());
	m_box2dRenderer.RenderWorld(physicsWorld.GetRawWorld(), physicsWorld.GetPixelsPerMeter(), false, true, true, true);

	// Render level boundaries
    auto view = registry.view<Component::Level, Component::WorldTransform>();
	for (auto [entity, level, worldTransform] : view.each())
	{
		::Rectangle levelBounds { worldTransform.position.x, worldTransform.position.y, (float)level.width, (float)level.height };
		::DrawRectangleLinesEx(levelBounds, 2, ORANGE);
	}
	::EndMode2D();

    ::DrawFPS(10,10);
}
