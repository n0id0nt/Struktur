#pragma once

#include "Engine/ECS/SystemManager.h"
#include "Engine/Renderer/DebugRenderer.h"
#ifdef STRUKTUR_PHYSICS
	#include "Debug/Box2DBgfxDebugDraw.h"
	#include "box2d/box2d.h"
#endif

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
	void RenderSelectedEntityHighlight(GameContext &context);
	void RenderGrid(GameContext &context);
#ifdef STRUKTUR_WORLD2D
	void RenderLevelBounds(GameContext &context);
	void RenderLevelEntityPlaceholders(GameContext &context);
#endif
#ifdef STRUKTUR_PHYSICS
	void RenderPhysicsShapes(GameContext &context);
#endif

	// Lets other editor code (e.g. Debug::UIHierarchyWindow, highlighting a selected UIElement's bounds via
	// DebugRenderer::SetupUIView) reach the same DebugRenderer instance this system already owns, rather than
	// each needing its own white texture/sampler pair.
	Renderer::DebugRenderer &GetDebugRenderer()
	{
		return m_debugRenderer;
	}

	std::string Name() const override
	{
		return "Debug System";
	}

private:
	Renderer::DebugRenderer m_debugRenderer;
#ifdef STRUKTUR_PHYSICS
	Debug::Box2DBgfxDebugDraw m_box2dBgfxDebugDraw{m_debugRenderer};
#endif
};
}  // namespace System
}  // namespace Struktur
