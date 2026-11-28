#include "FitnessServices.h"

#include <cstdlib>

#include "Debug/Assertions.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/Fitness/Ble/BleCentralFactory.h"
#include "Engine/Fitness/Ble/SimulatedBleCentral.h"

namespace Struktur::Fitness
{
// --demo on the command line, or STRUKTUR_FITNESS_SIMULATE=1, swaps Bluetooth for a fake trainer, heart rate strap and
// power meter driven by a synthetic rider, so every fitness screen can be worked on without hardware.
static std::unique_ptr<IBleCentral> CreateCentral()
{
	const char* simulate = std::getenv("STRUKTUR_FITNESS_SIMULATE");
	if (Core::CommandLine::HasFlag("--demo"))
	{
		DEBUG_INFO("--demo - using simulated fitness devices and a synthetic rider");
		return std::make_unique<SimulatedBleCentral>();
	}
	if (simulate != nullptr && simulate[0] != '\0' && simulate[0] != '0')
	{
		DEBUG_INFO("STRUKTUR_FITNESS_SIMULATE is set - using simulated fitness devices");
		return std::make_unique<SimulatedBleCentral>();
	}
	return CreatePlatformBleCentral();
}

Services::Services()
{
	// Idle until the game asks it to scan or connect (see docs/SmartTrainer.md) - constructing it never touches the
	// radio. Non-Windows platforms get a backend that reports Bluetooth as unavailable.
	fitnessManager = std::make_unique<FitnessManager>(CreateCentral());
}

void Services::Shutdown()
{
	fitnessManager.reset();
}
}  // namespace Struktur::Fitness
