#pragma once

#include "Debug/Box2DDebugRenderer.h"
#include "Engine/ECS/SystemManager.h"
#include "box2d/box2d.h"
#include "raylib.h"

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

	std::string Name() const override
	{
		return "Debug System";
	}

   private:
	Debug::Box2DDebugRenderer m_box2dRenderer;
};
}  // namespace System
}  // namespace Struktur
