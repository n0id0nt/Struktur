#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Draws the procedural ride world (RideIntegration::RideService::RenderWorld) when a ride has one. Its bgfx views
// run before the UI views (GraphicsDevice's view order), so the HUD draws on top.
class RideWorldRenderSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "Ride World Render System";
	}
};
}  // namespace System
}  // namespace Struktur
