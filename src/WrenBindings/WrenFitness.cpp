// "fitness" module - the Wren face of Engine/Fitness/FitnessManager (smart trainer, heart rate strap, power meter).
// Everything is a static on class Fitness; roles are the strings "trainer", "heartRate" and "powerMeter".
// Nothing here calls back into Wren: changes arrive as fitness* engine events (see ECS/System/FitnessSystem.h), and
// these getters return a snapshot (maps/lists built fresh on each call) for the moment an event says to look.
#include <optional>
#include <string>

#include "Debug/Assertions.h"
#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/Callback/Variant.h"
#include "Engine/Fitness/Ble/SimulatedBleCentral.h"
#include "Engine/Fitness/FitnessManager.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "wren.hpp"

using Struktur::Callback::Variant;
using Struktur::Callback::VariantList;
using Struktur::Callback::VariantMap;
namespace Fitness = Struktur::Fitness;

static Struktur::GameContext& GetContext(WrenVM* vm)
{
	return *static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
}

static Fitness::FitnessManager& GetFitness(WrenVM* vm)
{
	return GetContext(vm).GetFitnessManager();
}

static double GetNow(WrenVM* vm)
{
	return GetContext(vm).GetTimeSystem().unscaledTime;
}

static std::optional<Fitness::DeviceRole> RoleFromSlot(WrenVM* vm, int slot)
{
	if (wrenGetSlotType(vm, slot) != WREN_TYPE_STRING)
	{
		DEBUG_ERROR("Fitness: role must be a string (trainer, heartRate or powerMeter)");
		return std::nullopt;
	}
	const std::string text = wrenGetSlotString(vm, slot);
	for (Fitness::DeviceRole role :
	     {Fitness::DeviceRole::TRAINER, Fitness::DeviceRole::HEART_RATE, Fitness::DeviceRole::POWER_METER})
	{
		if (text == Fitness::ToString(role))
		{
			return role;
		}
	}
	DEBUG_ERROR("Fitness: unknown role '%s' (expected trainer, heartRate or powerMeter)", text.c_str());
	return std::nullopt;
}

static void ReturnVariant(WrenVM* vm, const Variant& value)
{
	Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, 0, value);
}

// Fitness.isAvailable()
static void wren_FitnessIsAvailable(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetFitness(vm).IsBluetoothAvailable());
}

// Fitness.isSimulated()
static void wren_FitnessIsSimulated(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, dynamic_cast<Fitness::SimulatedBleCentral*>(&GetFitness(vm).GetCentral()) != nullptr);
}

// Fitness.startScan()
static void wren_FitnessStartScan(WrenVM* vm)
{
	GetFitness(vm).StartScan();
}

// Fitness.stopScan()
static void wren_FitnessStopScan(WrenVM* vm)
{
	GetFitness(vm).StopScan();
}

// Fitness.isScanning()
static void wren_FitnessIsScanning(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetFitness(vm).IsScanning());
}

// Fitness.devices() -> [{"address", "name", "rssi", "roles": [role...], "inUseAs": role or null}]
static void wren_FitnessDevices(WrenVM* vm)
{
	Fitness::FitnessManager& fitness = GetFitness(vm);
	VariantList devices;
	for (const Fitness::DiscoveredDevice& discovered : fitness.GetDiscoveredDevices())
	{
		const std::string address = Fitness::FormatAddress(discovered.address);
		VariantMap device;
		device.items["address"] = address;
		device.items["name"]    = discovered.name;
		device.items["rssi"]    = static_cast<int>(discovered.rssi);
		VariantList roles;
		for (Fitness::DeviceRole role : discovered.roles)
		{
			roles.items.push_back(std::string(Fitness::ToString(role)));
		}
		device.items["roles"]   = roles;
		device.items["inUseAs"] = nullptr;
		for (Fitness::DeviceRole role :
		     {Fitness::DeviceRole::TRAINER, Fitness::DeviceRole::HEART_RATE, Fitness::DeviceRole::POWER_METER})
		{
			const Fitness::Device* assigned = fitness.GetDevice(role);
			if (assigned && assigned->GetIdentifier() == address)
			{
				device.items["inUseAs"] = std::string(Fitness::ToString(role));
			}
		}
		devices.items.push_back(device);
	}
	ReturnVariant(vm, devices);
}

// Fitness.assign(role, address, name) -> bool. Connects now and remembers the choice.
static void wren_FitnessAssign(WrenVM* vm)
{
	const std::optional<Fitness::DeviceRole> role = RoleFromSlot(vm, 1);
	Fitness::BleAddress address                   = 0;
	if (!role || wrenGetSlotType(vm, 2) != WREN_TYPE_STRING ||
	    !Fitness::ParseAddress(wrenGetSlotString(vm, 2), address))
	{
		DEBUG_ERROR("Fitness.assign: expected (role, \"AA:BB:CC:DD:EE:FF\", name)");
		wrenSetSlotBool(vm, 0, false);
		return;
	}
	const std::string name = wrenGetSlotType(vm, 3) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 3) : "";
	wrenSetSlotBool(vm, 0, GetFitness(vm).AssignDevice(*role, address, name, GetNow(vm)));
}

// Fitness.unassign(role) - disconnects (a trainer releases its load first) and forgets the device.
static void wren_FitnessUnassign(WrenVM* vm)
{
	if (const std::optional<Fitness::DeviceRole> role = RoleFromSlot(vm, 1))
	{
		GetFitness(vm).UnassignDevice(*role, true);
	}
}

// Fitness.connectRemembered() - starts every remembered device that isn't already running.
static void wren_FitnessConnectRemembered(WrenVM* vm)
{
	GetFitness(vm).ConnectRememberedDevices(GetNow(vm));
}

// Fitness.remembered(role) -> {"address", "name"} or null
static void wren_FitnessRemembered(WrenVM* vm)
{
	const std::optional<Fitness::DeviceRole> role = RoleFromSlot(vm, 1);
	const std::optional<Fitness::RememberedDevice> remembered =
	    role ? GetFitness(vm).GetRegistry().Get(*role) : std::nullopt;
	if (!remembered)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	VariantMap device;
	device.items["address"] = remembered->identifier;
	device.items["name"]    = remembered->name;
	ReturnVariant(vm, device);
}

// Fitness.status(role) -> {"state", "name", "address"} (+ "control", "mode" for the trainer) or null if unassigned.
// state: disconnected / connecting / connected / reconnecting / failed / stopped.
static void wren_FitnessStatus(WrenVM* vm)
{
	const std::optional<Fitness::DeviceRole> role = RoleFromSlot(vm, 1);
	const Fitness::Device* device                 = role ? GetFitness(vm).GetDevice(*role) : nullptr;
	if (!device)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	VariantMap status;
	status.items["state"]   = std::string(Fitness::ToString(device->GetConnectionState()));
	status.items["name"]    = device->GetName();
	status.items["address"] = device->GetIdentifier();
	if (const Fitness::Trainer* trainer = GetFitness(vm).GetTrainer(); trainer && *role == Fitness::DeviceRole::TRAINER)
	{
		status.items["control"] = std::string(Fitness::ToString(trainer->GetControlState()));
		status.items["mode"]    = std::string(Fitness::ToString(trainer->GetMode()));
	}
	ReturnVariant(vm, status);
}

// Fitness.sample(role) -> the latest values the device has reported (missing keys = never reported). Same keys as
// the fitnessSample event: trainer power/cadence/speed/heartRate, heartRate bpm, powerMeter power/cadence.
static void wren_FitnessSample(WrenVM* vm)
{
	const std::optional<Fitness::DeviceRole> role = RoleFromSlot(vm, 1);
	Fitness::FitnessManager& fitness              = GetFitness(vm);
	VariantMap sample;
	if (role == Fitness::DeviceRole::TRAINER && fitness.GetTrainer())
	{
		const Fitness::TrainerSample& values = fitness.GetTrainer()->GetLatestSample();
		if (values.powerWatts)
		{
			sample.items["power"] = *values.powerWatts;
		}
		if (values.cadenceRpm)
		{
			sample.items["cadence"] = *values.cadenceRpm;
		}
		if (values.speedKmh)
		{
			sample.items["speed"] = *values.speedKmh;
		}
		if (values.heartRateBpm)
		{
			sample.items["heartRate"] = *values.heartRateBpm;
		}
	}
	else if (role == Fitness::DeviceRole::HEART_RATE && fitness.GetHeartRateSensor())
	{
		if (const std::optional<int> bpm = fitness.GetHeartRateSensor()->GetLatestSample().bpm)
		{
			sample.items["bpm"] = *bpm;
		}
	}
	else if (role == Fitness::DeviceRole::POWER_METER && fitness.GetPowerMeter())
	{
		const Fitness::PowerSample& values = fitness.GetPowerMeter()->GetLatestSample();
		if (values.powerWatts)
		{
			sample.items["power"] = *values.powerWatts;
		}
		if (values.cadenceRpm)
		{
			sample.items["cadence"] = *values.cadenceRpm;
		}
	}
	ReturnVariant(vm, sample);
}

// Fitness.hasPowerSource() -> true when a trainer or power meter is connected.
static void wren_FitnessHasPowerSource(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetFitness(vm).HasConnectedPowerSource());
}

static void ReturnCommandResult(WrenVM* vm, const Fitness::CommandResult& result)
{
	wrenSetSlotString(vm, 0, Fitness::ToString(result.status));
}

// Fitness.setTargetPower(watts) -> "queued" / "clamped" / "deferred" / "unsupported" / "controlDenied" / "noTrainer"
static void wren_FitnessSetTargetPower(WrenVM* vm)
{
	Fitness::Trainer* trainer = GetFitness(vm).GetTrainer();
	if (!trainer)
	{
		wrenSetSlotString(vm, 0, "noTrainer");
		return;
	}
	ReturnCommandResult(vm, trainer->SetTargetPower(static_cast<int>(wrenGetSlotDouble(vm, 1))));
}

// Fitness.setGrade(percent) -> same results as setTargetPower
static void wren_FitnessSetGrade(WrenVM* vm)
{
	Fitness::Trainer* trainer = GetFitness(vm).GetTrainer();
	if (!trainer)
	{
		wrenSetSlotString(vm, 0, "noTrainer");
		return;
	}
	Fitness::SimulationTarget target;
	target.gradePercent = wrenGetSlotDouble(vm, 1);
	ReturnCommandResult(vm, trainer->SetSimulation(target));
}

// Fitness.setManualResistance(fraction 0..1) -> same results as setTargetPower
static void wren_FitnessSetManualResistance(WrenVM* vm)
{
	Fitness::Trainer* trainer = GetFitness(vm).GetTrainer();
	if (!trainer)
	{
		wrenSetSlotString(vm, 0, "noTrainer");
		return;
	}
	ReturnCommandResult(vm, trainer->SetManualResistance(wrenGetSlotDouble(vm, 1)));
}

// Fitness.clearTarget() -> same results as setTargetPower
static void wren_FitnessClearTarget(WrenVM* vm)
{
	Fitness::Trainer* trainer = GetFitness(vm).GetTrainer();
	if (!trainer)
	{
		wrenSetSlotString(vm, 0, "noTrainer");
		return;
	}
	ReturnCommandResult(vm, trainer->ClearTarget());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

WREN_BINDING_MODULE(Fitness)
{
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "isAvailable()", wren_FitnessIsAvailable,
	                  "True when a Bluetooth LE adapter (or the simulated backend) is usable.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "isSimulated()", wren_FitnessIsSimulated,
	                  "True when STRUKTUR_FITNESS_SIMULATE replaced Bluetooth with fake devices.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "startScan()", wren_FitnessStartScan,
	                  "Starts scanning for trainers, heart rate monitors and power meters.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "stopScan()", wren_FitnessStopScan, "Stops scanning.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "isScanning()", wren_FitnessIsScanning,
	                  "True while a scan is running.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "devices()", wren_FitnessDevices,
	                  "Devices found by scanning: list of maps (address, name, rssi, roles, inUseAs).");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "assign(_,_,_)", wren_FitnessAssign,
	                  "Connects a device in a role (role, address, name) and remembers it. Returns false if refused.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "unassign(_)", wren_FitnessUnassign,
	                  "Disconnects and forgets the device in a role.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "connectRemembered()", wren_FitnessConnectRemembered,
	                  "Connects every remembered device that is not already connected.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "remembered(_)", wren_FitnessRemembered,
	                  "The remembered device for a role (address, name) or null.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "status(_)", wren_FitnessStatus,
	                  "Connection status of a role's device (state, name, address, control, mode) or null.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "sample(_)", wren_FitnessSample,
	                  "Latest values reported by a role's device, as a map.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "hasPowerSource()", wren_FitnessHasPowerSource,
	                  "True when a trainer or power meter is connected.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "setTargetPower(_)", wren_FitnessSetTargetPower,
	                  "ERG target in watts. Returns the command result as a string.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "setGrade(_)", wren_FitnessSetGrade,
	                  "Simulation grade in percent. Returns the command result as a string.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "setManualResistance(_)", wren_FitnessSetManualResistance,
	                  "Manual resistance 0..1 (simulation grade when supported). Returns the command result.");
	WREN_CLASS_STATIC(registry, "fitness", "Fitness", "clearTarget()", wren_FitnessClearTarget,
	                  "Removes the trainer target (free ride). Returns the command result as a string.");
}
