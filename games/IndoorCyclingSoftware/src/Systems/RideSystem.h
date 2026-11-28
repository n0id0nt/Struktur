#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Ticks the active ride (RideIntegration::RideService) once per frame, after FitnessSystem has pumped the sensors:
// writes Input's trainer data, and links the ride to the world (road grade, finish banner).
class RideSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "Ride System";
	}
};
}  // namespace System
}  // namespace Struktur
