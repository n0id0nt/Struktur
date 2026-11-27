#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Draws the world while one is loaded. Its bgfx views run before the UI views (GraphicsDevice's view order), so the
// HUD draws on top.
class WorldRenderSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "World Render System";
	}
};
}  // namespace System
}  // namespace Struktur
