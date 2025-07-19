#include "DebugSystem.h"

#include "raylib.h"

#include "Engine/GameContext.h"

Struktur::System::DebugSystem::DebugSystem() : m_box2dRenderer()
{
}

void Struktur::System::DebugSystem::Update(GameContext &context)
{
	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	GameResource::Camera& camera = context.GetCamera();

	::BeginMode2D(camera.GetRaylibCamera());
	m_box2dRenderer.RenderWorld(physicsWorld.GetRawWorld(), physicsWorld.GetPixelsPerMeter(), false, true, true, true);
	::EndMode2D();

    ::DrawFPS(10,10);
}
