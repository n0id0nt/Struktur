#pragma once

#include <memory>

#include "Engine/Fitness/FitnessManager.h"

namespace Struktur::Fitness
{
// The fitness library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<FitnessManager> fitnessManager;
};
}  // namespace Struktur::Fitness
