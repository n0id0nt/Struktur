#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Advances the loaded world from the trainer input, once per frame after RideSystem has written it.
class WorldSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "World System";
	}
};
}  // namespace System
}  // namespace Struktur
