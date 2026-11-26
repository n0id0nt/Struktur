#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Fitness/Ble/FakeBleCentral.h"
#include "Engine/Fitness/Protocol/Ftms.h"

namespace Struktur::Fitness
{
// A scriptable FTMS trainer living on a FakeBleCentral - the "mock trainer" for unit tests and
// trainer_probe --simulate. Implements the FTMS control point state machine from the spec: Request Control gate,
// feature-gated op codes, parameter length checks, and Response Code indications.
class FakeFtmsTrainer
{
public:
	struct Config
	{
		BleAddress address       = 0xC0FFEE000001ull;
		std::string name         = "Fake FTMS Trainer";
		uint32_t machineFeatures = Ftms::MachineFeature::CADENCE | Ftms::MachineFeature::POWER_MEASUREMENT;
		uint32_t targetFeatures =
		    Ftms::TargetSettingFeature::POWER | Ftms::TargetSettingFeature::INDOOR_BIKE_SIMULATION;
		std::optional<Ftms::PowerRange> powerRange = Ftms::PowerRange{0, 2000, 1};
		// Raw sint16 tenths, as on the wire.
		std::optional<std::array<int16_t, 3>> resistanceRangeTenths;
		bool hasMachineStatus   = true;
		bool alsoAdvertisePower = true;  // Direto and KICKR both advertise Cycling Power alongside FTMS
		int maxConnections      = 1;
	};

	FakeFtmsTrainer(FakeBleCentral& central, Config config);

	// Emits one Indoor Bike Data notification derived from the current mode (ERG -> target power).
	void Tick(double nowSeconds);

	// --- fault injection -------------------------------------------------------------------------------------
	// false = writes are acknowledged but never answered (the reported Direto X "ERG does nothing" case).
	void SetRespondToControlPoint(bool respond);
	// Forces a result code for one op code.
	void SetResultOverride(Ftms::OpCode opCode, Ftms::ResultCode result);
	void ClearResultOverrides();
	// Sends Machine Status "Control Permission Lost" and drops control.
	void RevokeControl();

	// --- inspection ------------------------------------------------------------------------------------------
	bool HasControl() const;
	std::optional<int> GetTargetPower() const;
	std::optional<double> GetTargetGradePercent() const;
	std::optional<double> GetTargetResistance() const;
	const std::vector<Bytes>& GetReceivedCommands() const;
	std::vector<Ftms::OpCode> GetReceivedOpCodes() const;
	BleAddress GetAddress() const;

private:
	BleStatus OnWrite(const BleCharacteristicRef& characteristic, const Bytes& value);
	Ftms::ResultCode Execute(const Bytes& value);

	FakeBleCentral* m_central;
	Config m_config;
	bool m_respond;
	bool m_hasControl;
	std::optional<int> m_targetPower;
	std::optional<double> m_targetGrade;
	std::optional<double> m_targetResistance;
	std::map<uint8_t, Ftms::ResultCode> m_overrides;
	std::vector<Bytes> m_commands;
	double m_distanceMeters;
	double m_lastTickSeconds;
};

// Plain sensors for simulate mode / tests. Each Tick sends one measurement.
void AddFakeHeartRateMonitor(FakeBleCentral& central, BleAddress address, const std::string& name);
void SendFakeHeartRate(FakeBleCentral& central, BleAddress address, uint8_t bpm);
void AddFakePowerMeter(FakeBleCentral& central, BleAddress address, const std::string& name);
void SendFakePower(FakeBleCentral& central, BleAddress address, int16_t watts, uint16_t crankRevolutions,
                   uint16_t crankEventTime1024);
}  // namespace Struktur::Fitness
