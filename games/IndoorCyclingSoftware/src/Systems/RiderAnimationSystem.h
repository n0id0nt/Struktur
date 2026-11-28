#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Animates every rider: the player's from the trainer input, pacers from their own cadence.
class RiderAnimationSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	SystemAccess Access() const override;
	void Prepare(GameContext& context) override;

	std::string Name() const override
	{
		return "Rider Animation System";
	}
};
}  // namespace System
}  // namespace Struktur
