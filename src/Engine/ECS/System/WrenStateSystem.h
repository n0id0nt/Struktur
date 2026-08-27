#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
class WrenStateSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Wren State System";
	}
};

class WrenStateRenderSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Wren State Render System";
	}
};

// Registered via SystemManager::AddFixedUpdateSystem - a second dispatch pass over the same WrenStateManager
// used by WrenStateSystem above, running at the fixed cadence (see GameLoop's accumulator loop) instead of once
// per render frame. Mirrors WrenStateRenderSystem's own shape.
class WrenStateFixedUpdateSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Wren State Fixed Update System";
	}
};
}  // namespace System
}  // namespace Struktur
