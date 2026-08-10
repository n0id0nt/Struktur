#pragma once

#include "Debug/Box2DBgfxDebugDraw.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Renderer/DebugRenderer.h"
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

	std::string Name() const override
	{
		return "Debug System";
	}

   private:
	Renderer::DebugRenderer m_debugRenderer;
	Debug::Box2DBgfxDebugDraw m_box2dBgfxDebugDraw{m_debugRenderer};
};
}  // namespace System
}  // namespace Struktur
