#include "FakeDevices.h"

#include <algorithm>
#include <cmath>

namespace Struktur::Fitness
{
static const BleUuid FTMS_SERVICE = BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE);
static const BleUuid CPS_SERVICE  = BleUuid::FromShort(Uuids::CYCLING_POWER_SERVICE);
static const BleUuid HR_SERVICE   = BleUuid::FromShort(Uuids::HEART_RATE_SERVICE);
static const BleCharacteristicRef FEATURE{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_FEATURE)};
static const BleCharacteristicRef POWER_RANGE{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_SUPPORTED_POWER)};
static const BleCharacteristicRef RESISTANCE_RANGE{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_SUPPORTED_RESISTANCE)};
static const BleCharacteristicRef INDOOR_BIKE_DATA{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_INDOOR_BIKE_DATA)};
static const BleCharacteristicRef CONTROL_POINT{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_CONTROL_POINT)};
static const BleCharacteristicRef MACHINE_STATUS{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_MACHINE_STATUS)};
static const BleCharacteristicRef CPS_MEASUREMENT{CPS_SERVICE, BleUuid::FromShort(Uuids::CYCLING_POWER_MEASUREMENT)};
static const BleCharacteristicRef HR_MEASUREMENT{HR_SERVICE, BleUuid::FromShort(Uuids::HEART_RATE_MEASUREMENT)};

static void AppendU16(Bytes& bytes, uint16_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFF));
	bytes.push_back(static_cast<uint8_t>(value >> 8));
}

static void AppendU32(Bytes& bytes, uint32_t value)
{
	AppendU16(bytes, static_cast<uint16_t>(value & 0xFFFF));
	AppendU16(bytes, static_cast<uint16_t>(value >> 16));
}

static int16_t ReadS16(const Bytes& bytes, size_t offset)
{
	return static_cast<int16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

FakeFtmsTrainer::FakeFtmsTrainer(FakeBleCentral& central, Config config)
    : m_central(&central),
      m_config(std::move(config)),
      m_respond(true),
      m_hasControl(false),
      m_targetPower(),
      m_targetGrade(),
      m_targetResistance(),
      m_overrides(),
      m_commands(),
      m_distanceMeters(0.0),
      m_lastTickSeconds(0.0)
{
	std::vector<BleUuid> advertised = {FTMS_SERVICE};
	if (m_config.alsoAdvertisePower)
	{
		advertised.push_back(CPS_SERVICE);
	}
	FakeBleCentral::Peripheral& peripheral = central.AddPeripheral(m_config.address, m_config.name, advertised);
	peripheral.maxConnections              = m_config.maxConnections;
	peripheral.onWrite                     = [this](const BleCharacteristicRef& characteristic, const Bytes& value)
	{ return OnWrite(characteristic, value); };

	Bytes feature;
	AppendU32(feature, m_config.machineFeatures);
	AppendU32(feature, m_config.targetFeatures);
	central.AddCharacteristic(m_config.address, FEATURE, CharacteristicProperty::READ, feature);

	if (m_config.powerRange)
	{
		Bytes range;
		AppendU16(range, static_cast<uint16_t>(m_config.powerRange->minimumWatts));
		AppendU16(range, static_cast<uint16_t>(m_config.powerRange->maximumWatts));
		AppendU16(range, m_config.powerRange->incrementWatts);
		central.AddCharacteristic(m_config.address, POWER_RANGE, CharacteristicProperty::READ, range);
	}
	if (m_config.resistanceRangeTenths)
	{
		Bytes range;
		for (int16_t value : *m_config.resistanceRangeTenths)
		{
			AppendU16(range, static_cast<uint16_t>(value));
		}
		central.AddCharacteristic(m_config.address, RESISTANCE_RANGE, CharacteristicProperty::READ, range);
	}
	central.AddCharacteristic(m_config.address, INDOOR_BIKE_DATA, CharacteristicProperty::NOTIFY);
	central.AddCharacteristic(m_config.address, CONTROL_POINT,
	                          CharacteristicProperty::WRITE | CharacteristicProperty::INDICATE);
	if (m_config.hasMachineStatus)
	{
		central.AddCharacteristic(m_config.address, MACHINE_STATUS, CharacteristicProperty::NOTIFY);
	}
	if (m_config.alsoAdvertisePower)
	{
		central.AddCharacteristic(m_config.address, CPS_MEASUREMENT, CharacteristicProperty::NOTIFY);
	}
}

void FakeFtmsTrainer::Tick(double nowSeconds)
{
	// Crude but plausible rider: 90 rpm; ERG holds the target, simulation adds load with grade.
	int power = 150;
	if (m_targetPower)
	{
		power = *m_targetPower;
	}
	else if (m_targetGrade)
	{
		power = std::max(0, static_cast<int>(150 + *m_targetGrade * 20.0));
	}
	const double speedKmh = 20.0 + power / 20.0;
	const double elapsed  = m_lastTickSeconds > 0.0 ? nowSeconds - m_lastTickSeconds : 0.0;
	m_lastTickSeconds     = nowSeconds;
	m_distanceMeters += speedKmh / 3.6 * std::max(0.0, elapsed);

	using namespace Ftms::IndoorBikeDataFlags;
	Bytes data;
	AppendU16(data, INSTANTANEOUS_CADENCE | TOTAL_DISTANCE | INSTANTANEOUS_POWER);
	AppendU16(data, static_cast<uint16_t>(std::lround(speedKmh * 100.0)));
	AppendU16(data, 180);  // 90 rpm in 0.5 rpm units
	const uint32_t distance = static_cast<uint32_t>(m_distanceMeters);
	data.push_back(static_cast<uint8_t>(distance & 0xFF));
	data.push_back(static_cast<uint8_t>((distance >> 8) & 0xFF));
	data.push_back(static_cast<uint8_t>((distance >> 16) & 0xFF));
	AppendU16(data, static_cast<uint16_t>(static_cast<int16_t>(power)));
	m_central->Notify(m_config.address, INDOOR_BIKE_DATA, std::move(data));
}

void FakeFtmsTrainer::SetRespondToControlPoint(bool respond)
{
	m_respond = respond;
}

void FakeFtmsTrainer::SetResultOverride(Ftms::OpCode opCode, Ftms::ResultCode result)
{
	m_overrides[static_cast<uint8_t>(opCode)] = result;
}

void FakeFtmsTrainer::ClearResultOverrides()
{
	m_overrides.clear();
}

void FakeFtmsTrainer::RevokeControl()
{
	m_hasControl = false;
	m_central->Notify(m_config.address, MACHINE_STATUS, Bytes{Ftms::MachineStatusOpCode::CONTROL_PERMISSION_LOST});
}

bool FakeFtmsTrainer::HasControl() const
{
	return m_hasControl;
}

std::optional<int> FakeFtmsTrainer::GetTargetPower() const
{
	return m_targetPower;
}

std::optional<double> FakeFtmsTrainer::GetTargetGradePercent() const
{
	return m_targetGrade;
}

std::optional<double> FakeFtmsTrainer::GetTargetResistance() const
{
	return m_targetResistance;
}

const std::vector<Bytes>& FakeFtmsTrainer::GetReceivedCommands() const
{
	return m_commands;
}

std::vector<Ftms::OpCode> FakeFtmsTrainer::GetReceivedOpCodes() const
{
	std::vector<Ftms::OpCode> opCodes;
	for (const Bytes& command : m_commands)
	{
		if (!command.empty())
		{
			opCodes.push_back(static_cast<Ftms::OpCode>(command[0]));
		}
	}
	return opCodes;
}

BleAddress FakeFtmsTrainer::GetAddress() const
{
	return m_config.address;
}

BleStatus FakeFtmsTrainer::OnWrite(const BleCharacteristicRef& characteristic, const Bytes& value)
{
	if (characteristic != CONTROL_POINT)
	{
		return BleStatus::PROTOCOL_ERROR;
	}
	m_commands.push_back(value);
	if (value.empty())
	{
		return BleStatus::PROTOCOL_ERROR;
	}
	const Ftms::ResultCode result = Execute(value);
	if (m_respond)
	{
		m_central->Notify(m_config.address, CONTROL_POINT,
		                  Bytes{static_cast<uint8_t>(Ftms::OpCode::RESPONSE_CODE), value[0],
		                        static_cast<uint8_t>(result)});
	}
	return BleStatus::SUCCESS;
}

Ftms::ResultCode FakeFtmsTrainer::Execute(const Bytes& value)
{
	const uint8_t opCode = value[0];
	auto overridden      = m_overrides.find(opCode);
	if (overridden != m_overrides.end())
	{
		if (opCode == static_cast<uint8_t>(Ftms::OpCode::REQUEST_CONTROL) &&
		    overridden->second == Ftms::ResultCode::SUCCESS)
		{
			m_hasControl = true;
		}
		return overridden->second;
	}

	using Ftms::OpCode;
	using Ftms::ResultCode;
	const auto hasTarget = [this](uint32_t bit) { return (m_config.targetFeatures & bit) != 0; };

	if (opCode == static_cast<uint8_t>(OpCode::REQUEST_CONTROL))
	{
		m_hasControl = true;
		return ResultCode::SUCCESS;
	}
	if (!m_hasControl)
	{
		return ResultCode::CONTROL_NOT_PERMITTED;
	}

	switch (static_cast<OpCode>(opCode))
	{
		case OpCode::RESET:
			m_hasControl = false;
			m_targetPower.reset();
			m_targetGrade.reset();
			m_targetResistance.reset();
			return ResultCode::SUCCESS;
		case OpCode::START_OR_RESUME:
			return ResultCode::SUCCESS;
		case OpCode::STOP_OR_PAUSE:
			return value.size() == 2 ? ResultCode::SUCCESS : ResultCode::INVALID_PARAMETER;
		case OpCode::SET_TARGET_POWER:
			if (!hasTarget(Ftms::TargetSettingFeature::POWER))
			{
				return ResultCode::OP_CODE_NOT_SUPPORTED;
			}
			if (value.size() != 3)
			{
				return ResultCode::INVALID_PARAMETER;
			}
			m_targetPower = ReadS16(value, 1);
			m_targetGrade.reset();
			m_targetResistance.reset();
			return ResultCode::SUCCESS;
		case OpCode::SET_TARGET_RESISTANCE_LEVEL:
			if (!hasTarget(Ftms::TargetSettingFeature::RESISTANCE))
			{
				return ResultCode::OP_CODE_NOT_SUPPORTED;
			}
			if (value.size() != 2)
			{
				return ResultCode::INVALID_PARAMETER;
			}
			m_targetResistance = value[1] * 0.1;
			m_targetPower.reset();
			m_targetGrade.reset();
			return ResultCode::SUCCESS;
		case OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS:
			if (!hasTarget(Ftms::TargetSettingFeature::INDOOR_BIKE_SIMULATION))
			{
				return ResultCode::OP_CODE_NOT_SUPPORTED;
			}
			if (value.size() != 7)
			{
				return ResultCode::INVALID_PARAMETER;
			}
			m_targetGrade = ReadS16(value, 3) * 0.01;
			m_targetPower.reset();
			m_targetResistance.reset();
			return ResultCode::SUCCESS;
		default:
			return ResultCode::OP_CODE_NOT_SUPPORTED;
	}
}

void AddFakeHeartRateMonitor(FakeBleCentral& central, BleAddress address, const std::string& name)
{
	central.AddPeripheral(address, name, {HR_SERVICE});
	central.AddCharacteristic(address, HR_MEASUREMENT, CharacteristicProperty::NOTIFY);
}

void SendFakeHeartRate(FakeBleCentral& central, BleAddress address, uint8_t bpm)
{
	// flags 0x06: uint8 value, sensor contact supported + detected
	central.Notify(address, HR_MEASUREMENT, Bytes{0x06, bpm});
}

void AddFakePowerMeter(FakeBleCentral& central, BleAddress address, const std::string& name)
{
	central.AddPeripheral(address, name, {CPS_SERVICE});
	central.AddCharacteristic(address, CPS_MEASUREMENT, CharacteristicProperty::NOTIFY);
}

void SendFakePower(FakeBleCentral& central, BleAddress address, int16_t watts, uint16_t crankRevolutions,
                   uint16_t crankEventTime1024)
{
	Bytes data;
	AppendU16(data, 0x0020);  // crank revolution data present
	AppendU16(data, static_cast<uint16_t>(watts));
	AppendU16(data, crankRevolutions);
	AppendU16(data, crankEventTime1024);
	central.Notify(address, CPS_MEASUREMENT, std::move(data));
}
}  // namespace Struktur::Fitness
