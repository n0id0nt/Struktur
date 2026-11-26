#include "FitnessSystem.h"

#include <cctype>
#include <string>

#include "Engine/Callback/Variant.h"
#include "Engine/Fitness/FitnessManager.h"
#include "Engine/GameContext.h"

namespace Struktur::System
{
static std::string EventTypeName(Fitness::DeviceEvent::Type type)
{
	std::string name = Fitness::ToString(type);
	if (!name.empty())
	{
		name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
	}
	return "fitness" + name;
}

static void AddSampleFields(const Fitness::FitnessManager& manager, Fitness::DeviceRole role,
                            Callback::VariantMap& data)
{
	switch (role)
	{
		case Fitness::DeviceRole::TRAINER:
			if (const Fitness::Trainer* trainer = manager.GetTrainer())
			{
				const Fitness::TrainerSample& sample = trainer->GetLatestSample();
				if (sample.powerWatts)
				{
					data.items["power"] = *sample.powerWatts;
				}
				if (sample.cadenceRpm)
				{
					data.items["cadence"] = *sample.cadenceRpm;
				}
				if (sample.speedKmh)
				{
					data.items["speed"] = *sample.speedKmh;
				}
				if (sample.heartRateBpm)
				{
					data.items["heartRate"] = *sample.heartRateBpm;
				}
			}
			return;
		case Fitness::DeviceRole::HEART_RATE:
			if (const Fitness::HeartRateSensor* sensor = manager.GetHeartRateSensor())
			{
				if (sensor->GetLatestSample().bpm)
				{
					data.items["bpm"] = *sensor->GetLatestSample().bpm;
				}
			}
			return;
		case Fitness::DeviceRole::POWER_METER:
			if (const Fitness::PowerMeter* meter = manager.GetPowerMeter())
			{
				const Fitness::PowerSample& sample = meter->GetLatestSample();
				if (sample.powerWatts)
				{
					data.items["power"] = *sample.powerWatts;
				}
				if (sample.cadenceRpm)
				{
					data.items["cadence"] = *sample.cadenceRpm;
				}
			}
			return;
	}
}

void FitnessSystem::Update(GameContext& context)
{
	Fitness::FitnessManager& fitnessManager = context.GetFitnessManager();
	fitnessManager.Update(context.GetTimeSystem().unscaledTime);

	Event::EventManager& eventManager = context.GetEventManager();
	for (const Fitness::DeviceEvent& event : fitnessManager.TakeEvents())
	{
		Callback::VariantMap data;
		data.items["role"]   = std::string(Fitness::ToString(event.role));
		data.items["detail"] = event.detail;
		if (const Fitness::Device* device = fitnessManager.GetDevice(event.role))
		{
			data.items["device"] = device->GetIdentifier();
			data.items["name"]   = device->GetName();
		}
		if (event.type == Fitness::DeviceEvent::Type::SAMPLE)
		{
			AddSampleFields(fitnessManager, event.role, data);
		}
		eventManager.AddEvent(EventTypeName(event.type), data);
	}
}
}  // namespace Struktur::System
