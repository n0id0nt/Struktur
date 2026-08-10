#pragma once

#include "Debug/Box2DBgfxDebugDraw.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Renderer/DebugRenderer.h"
#include "Engine/Resource/FontResource.h"
#include "box2d/box2d.h"

namespace Struktur
{
class GameContext;

namespace System
{
class DebugSystem : public ISystem
{
   public:
	DebugSystem();

	void Update(GameContext &context) override;

	void RenderEntityGizmos(GameContext &context);
	void RenderGrid(GameContext &context);
	void RenderLevelBounds(GameContext &context);

	// Releases the cached FPS font's ResourcePtr - must be called (see Game.cpp's ClearGameSystems) before
	// ResourceManager::Clear() runs, since that unconditionally frees every pooled resource regardless of
	// outstanding references; leaving m_fpsFont pointed at freed memory across a Play-button restart is a
	// use-after-free the moment Update() next dereferences it.
	void ClearCachedResources()
	{
		m_fpsFont = Resource::ResourcePtr<Resource::FontResource>();
	}

	std::string Name() const override
	{
		return "Debug System";
	}

   private:
	Renderer::DebugRenderer m_debugRenderer;
	Debug::Box2DBgfxDebugDraw m_box2dBgfxDebugDraw{m_debugRenderer};
	// Fetched once on first showFPS draw and held for the system's lifetime - avoids doing a ResourcePool
	// lookup (map lookup + refcount churn) on every single frame just to draw the FPS counter.
	Resource::ResourcePtr<Resource::FontResource> m_fpsFont;
};
}  // namespace System
}  // namespace Struktur
