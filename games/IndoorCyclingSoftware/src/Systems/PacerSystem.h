#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Moves the pacer riders along the road (World::MovePacer).
class PacerSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	SystemAccess Access() const override;
	void Prepare(GameContext& context) override;

	std::string Name() const override
	{
		return "Pacer System";
	}
};
}  // namespace System
}  // namespace Struktur
