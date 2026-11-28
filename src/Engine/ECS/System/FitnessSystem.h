#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
// Pumps the FitnessManager once per frame (BLE results reach the game thread here) and forwards every device event
// to the engine EventManager, so Wren reacts through onEvent rather than polling:
//
//   type  "fitness" + event name, e.g. fitnessConnected, fitnessSample, fitnessControlGranted, fitnessCommandFailed
//   data  {"role": "trainer" | "heartRate" | "powerMeter", "detail": String, "device": "AA:BB:...", "name": String}
//         fitnessSample also carries the snapshot: trainer -> power, cadence, speed, heartRate; heartRate -> bpm;
//         powerMeter -> power, cadence (only the fields the device has reported).
//   plus  fitnessDeviceFound while scanning, once per newly seen (or newly named) device:
//         {"address": String, "name": String, "rssi": Num, "roles": [role...]}
class FitnessSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "Fitness System";
	}
};
}  // namespace System
}  // namespace Struktur
