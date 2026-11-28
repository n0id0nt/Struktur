// trainer_probe - scan for, connect to and exercise BLE smart trainers, heart rate monitors and power meters using
// the engine's StrukturFitness library. See docs/SmartTrainer.md ("Running trainer_probe").
//
// Safety: target power is capped at 100 W unless --allow-high-power is given. The cap is enforced inside the
// trainer layer (FtmsTrainerSettings::maximumTargetPowerWatts), not just by argument parsing.

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "Engine/Fitness/Ble/BleCentralFactory.h"
#include "Engine/Fitness/Ble/FakeBleCentral.h"
#include "Engine/Fitness/Ble/FakeDevices.h"
#include "Engine/Fitness/FitnessManager.h"

using namespace Struktur::Fitness;

static constexpr int DEFAULT_POWER_CAP_WATTS = 100;

static std::atomic<bool> g_stopRequested{false};

static void OnSignal(int)
{
	g_stopRequested = true;
}

struct ProbeOptions
{
	double scanSeconds = 8.0;
	std::optional<double> durationSeconds;
	std::optional<std::string> trainerAddress;
	std::optional<std::string> heartRateAddress;
	std::optional<std::string> powerMeterAddress;
	bool autoConnect = false;
	std::optional<int> targetPowerWatts;
	std::optional<double> gradePercent;
	std::optional<double> manualResistance;
	std::optional<int> maxPowerWatts;
	bool allowHighPower = false;
	bool simulate       = false;
	std::string registryPath;
	bool useRegistry = false;
};

static void PrintUsage()
{
	std::printf(
	    "Usage: trainer_probe [options]\n"
	    "\n"
	    "With no device options: scan for trainers (FTMS), heart rate monitors and power meters, list them, exit.\n"
	    "\n"
	    "Devices:\n"
	    "  --trainer ADDR          connect to an FTMS trainer (ADDR = AA:BB:CC:DD:EE:FF)\n"
	    "  --hr ADDR               connect to a heart rate monitor\n"
	    "  --power ADDR            connect to a cycling power meter\n"
	    "  --auto                  scan, then connect the first device found for each role\n"
	    "  --remembered PATH       connect the devices remembered in a registry JSON (and save choices there)\n"
	    "\n"
	    "Trainer control (sent once control is granted):\n"
	    "  --target-power W        ERG target in watts (capped, see below)\n"
	    "  --grade PCT             simulation grade in percent\n"
	    "  --manual F              manual resistance 0..1 (simulation grade when supported)\n"
	    "  --max-power W           power cap; values above %d W also need --allow-high-power\n"
	    "  --allow-high-power      lift the default %d W cap (to --max-power, or the trainer's own maximum)\n"
	    "\n"
	    "General:\n"
	    "  --scan-seconds S        scan length (default 8)\n"
	    "  --duration S            stop after S seconds (default: run until Ctrl+C)\n"
	    "  --simulate              use a built-in fake trainer, HR strap and power meter (no Bluetooth needed)\n"
	    "  --help\n"
	    "\n"
	    "Ctrl+C stops cleanly: the trainer is sent Reset (drops ERG load, releases control) and every BLE link is\n"
	    "closed, so a single-connection trainer (Elite Direto) is free for other apps immediately.\n",
	    DEFAULT_POWER_CAP_WATTS, DEFAULT_POWER_CAP_WATTS);
}

static bool ParseDouble(const char* text, double& out)
{
	char* end = nullptr;
	out       = std::strtod(text, &end);
	return end && *end == '\0' && end != text;
}

static bool ParseInt(const char* text, int& out)
{
	char* end         = nullptr;
	const long parsed = std::strtol(text, &end, 10);
	out               = static_cast<int>(parsed);
	return end && *end == '\0' && end != text;
}

// Returns an exit code on failure, nullopt when parsing succeeded.
static std::optional<int> ParseArguments(int argc, char** argv, ProbeOptions& options)
{
	for (int i = 1; i < argc; ++i)
	{
		const std::string argument = argv[i];
		const bool hasValue        = i + 1 < argc;
		auto missingValue          = [&argument]()
		{
			std::fprintf(stderr, "%s needs a value\n", argument.c_str());
			return 1;
		};

		if (argument == "--help" || argument == "-h")
		{
			PrintUsage();
			return 0;
		}
		else if (argument == "--auto")
		{
			options.autoConnect = true;
		}
		else if (argument == "--simulate")
		{
			options.simulate = true;
		}
		else if (argument == "--allow-high-power")
		{
			options.allowHighPower = true;
		}
		else if (argument == "--trainer" || argument == "--hr" || argument == "--power" || argument == "--remembered")
		{
			if (!hasValue)
			{
				return missingValue();
			}
			const std::string value = argv[++i];
			if (argument == "--trainer")
			{
				options.trainerAddress = value;
			}
			else if (argument == "--hr")
			{
				options.heartRateAddress = value;
			}
			else if (argument == "--power")
			{
				options.powerMeterAddress = value;
			}
			else
			{
				options.registryPath = value;
				options.useRegistry  = true;
			}
		}
		else if (argument == "--target-power" || argument == "--max-power")
		{
			int value = 0;
			if (!hasValue || !ParseInt(argv[++i], value) || value < 0)
			{
				std::fprintf(stderr, "%s needs a non-negative whole number of watts\n", argument.c_str());
				return 1;
			}
			(argument == "--target-power" ? options.targetPowerWatts : options.maxPowerWatts) = value;
		}
		else if (argument == "--grade" || argument == "--manual" || argument == "--scan-seconds" ||
		         argument == "--duration")
		{
			double value = 0.0;
			if (!hasValue || !ParseDouble(argv[++i], value))
			{
				std::fprintf(stderr, "%s needs a number\n", argument.c_str());
				return 1;
			}
			if (argument == "--grade")
			{
				options.gradePercent = value;
			}
			else if (argument == "--manual")
			{
				options.manualResistance = value;
			}
			else if (argument == "--scan-seconds")
			{
				options.scanSeconds = value;
			}
			else
			{
				options.durationSeconds = value;
			}
		}
		else
		{
			std::fprintf(stderr, "Unknown option '%s' (try --help)\n", argument.c_str());
			return 1;
		}
	}

	const int requestedCap = options.maxPowerWatts.value_or(DEFAULT_POWER_CAP_WATTS);
	if (requestedCap > DEFAULT_POWER_CAP_WATTS && !options.allowHighPower)
	{
		std::fprintf(stderr, "--max-power %d exceeds the %d W default cap; add --allow-high-power to confirm.\n",
		             requestedCap, DEFAULT_POWER_CAP_WATTS);
		return 1;
	}
	if (options.targetPowerWatts && !options.allowHighPower && *options.targetPowerWatts > requestedCap)
	{
		std::fprintf(stderr,
		             "--target-power %d exceeds the %d W cap; add --allow-high-power (and --max-power) to "
		             "go higher.\n",
		             *options.targetPowerWatts, requestedCap);
		return 1;
	}
	return std::nullopt;
}

static std::string RolesText(const std::vector<DeviceRole>& roles)
{
	std::string text;
	for (DeviceRole role : roles)
	{
		text += text.empty() ? "" : ",";
		text += ToString(role);
	}
	return text;
}

static void PrintEvent(const DeviceEvent& event, double now)
{
	if (event.type == DeviceEvent::Type::SAMPLE)
	{
		return;
	}
	std::printf("[%7.1fs] %-10s %-20s %s\n", now, ToString(event.role), ToString(event.type), event.detail.c_str());
}

static std::string FormatOptional(const std::optional<double>& value, const char* format)
{
	if (!value)
	{
		return "--";
	}
	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), format, *value);
	return std::string(buffer);
}

static void PrintStatus(FitnessManager& manager, double now)
{
	std::string line;
	char buffer[160];
	if (Trainer* trainer = manager.GetTrainer())
	{
		const TrainerSample& sample = trainer->GetLatestSample();
		std::snprintf(
		    buffer, sizeof(buffer), "trainer %s ctrl=%s mode=%s P=%sW cad=%srpm spd=%skm/h",
		    ToString(trainer->GetConnectionState()), ToString(trainer->GetControlState()), ToString(trainer->GetMode()),
		    FormatOptional(sample.powerWatts ? std::optional<double>(*sample.powerWatts) : std::nullopt, "%.0f")
		        .c_str(),
		    FormatOptional(sample.cadenceRpm, "%.0f").c_str(), FormatOptional(sample.speedKmh, "%.1f").c_str());
		line += buffer;
	}
	if (HeartRateSensor* sensor = manager.GetHeartRateSensor())
	{
		const HeartRateSample& sample = sensor->GetLatestSample();
		std::snprintf(buffer, sizeof(buffer), "%shr %s %sbpm", line.empty() ? "" : " | ",
		              ToString(sensor->GetConnectionState()),
		              FormatOptional(sample.bpm ? std::optional<double>(*sample.bpm) : std::nullopt, "%.0f").c_str());
		line += buffer;
	}
	if (PowerMeter* meter = manager.GetPowerMeter())
	{
		const PowerSample& sample = meter->GetLatestSample();
		std::snprintf(buffer, sizeof(buffer), "%spm %s %sW %srpm", line.empty() ? "" : " | ",
		              ToString(meter->GetConnectionState()),
		              FormatOptional(sample.powerWatts ? std::optional<double>(*sample.powerWatts) : std::nullopt,
		                             "%.0f")
		                  .c_str(),
		              FormatOptional(sample.cadenceRpm, "%.0f").c_str());
		line += buffer;
	}
	std::printf("[%7.1fs] %s\n", now, line.c_str());
}

// --simulate: a fake trainer, strap and power meter on a FakeBleCentral, ticked from the main loop.
struct Simulation
{
	FakeBleCentral* central = nullptr;
	std::unique_ptr<FakeFtmsTrainer> trainer;
	BleAddress heartRateAddress  = 0xC0FFEE000002ull;
	BleAddress powerMeterAddress = 0xC0FFEE000003ull;
	uint16_t crankRevolutions    = 0;
	uint16_t crankEventTime      = 0;

	void Tick(double now)
	{
		trainer->Tick(now);
		SendFakeHeartRate(*central, heartRateAddress, static_cast<uint8_t>(120 + static_cast<int>(now) % 20));
		crankRevolutions = static_cast<uint16_t>(crankRevolutions + 1);
		crankEventTime   = static_cast<uint16_t>(crankEventTime + 1024 * 60 / 85);  // ~85 rpm
		SendFakePower(*central, powerMeterAddress, 180, crankRevolutions, crankEventTime);
	}
};

int main(int argc, char** argv)
{
	ProbeOptions options;
	if (const std::optional<int> exitCode = ParseArguments(argc, argv, options))
	{
		return *exitCode;
	}
	std::signal(SIGINT, OnSignal);
	std::signal(SIGTERM, OnSignal);

	FitnessSettings settings;
	settings.trainer.maximumTargetPowerWatts =
	    options.allowHighPower ? options.maxPowerWatts
	                           : std::optional<int>(options.maxPowerWatts.value_or(DEFAULT_POWER_CAP_WATTS));
	settings.registryPath = options.registryPath;

	Simulation simulation;
	std::unique_ptr<IBleCentral> central;
	if (options.simulate)
	{
		auto fakeCentral   = std::make_unique<FakeBleCentral>();
		simulation.central = fakeCentral.get();
		FakeFtmsTrainer::Config config;
		config.name                  = "SIM DIRETO";
		config.resistanceRangeTenths = std::array<int16_t, 3>{0, 200, 10};
		simulation.trainer           = std::make_unique<FakeFtmsTrainer>(*fakeCentral, config);
		AddFakeHeartRateMonitor(*fakeCentral, simulation.heartRateAddress, "SIM HRM");
		AddFakePowerMeter(*fakeCentral, simulation.powerMeterAddress, "SIM PEDALS");
		central = std::move(fakeCentral);
	}
	else
	{
		central = CreatePlatformBleCentral();
	}

	FitnessManager manager(std::move(central), settings);
	if (!manager.IsBluetoothAvailable())
	{
		std::fprintf(stderr, "No usable Bluetooth LE adapter (or no BLE backend for this platform).\n");
		return 2;
	}

	const auto start = std::chrono::steady_clock::now();
	auto now = [&start]() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };

	if (settings.trainer.maximumTargetPowerWatts)
	{
		std::printf("Target power cap: %d W\n", *settings.trainer.maximumTargetPowerWatts);
	}
	else
	{
		std::printf("Target power cap: trainer maximum (--allow-high-power)\n");
	}

	// --simulate on its own connects all three fake devices.
	if (options.simulate && !options.trainerAddress && !options.heartRateAddress && !options.powerMeterAddress &&
	    !options.autoConnect && !options.useRegistry)
	{
		options.trainerAddress    = FormatAddress(simulation.trainer->GetAddress());
		options.heartRateAddress  = FormatAddress(simulation.heartRateAddress);
		options.powerMeterAddress = FormatAddress(simulation.powerMeterAddress);
	}

	const bool anyDevice = options.trainerAddress || options.heartRateAddress || options.powerMeterAddress ||
	                       options.autoConnect || options.useRegistry;

	// ---- scan ----------------------------------------------------------------------------------------------
	if (!anyDevice || options.autoConnect)
	{
		std::printf("Scanning %.0f s for FTMS (0x1826), heart rate (0x180D) and cycling power (0x1818)...\n",
		            options.scanSeconds);
		manager.StartScan();
		size_t printed = 0;
		while (now() < options.scanSeconds && !g_stopRequested)
		{
			manager.Update(now());
			if (options.simulate)
			{
				simulation.central->AdvertiseAll();
			}
			const std::vector<DiscoveredDevice>& devices = manager.GetDiscoveredDevices();
			for (; printed < devices.size(); ++printed)
			{
				const DiscoveredDevice& device = devices[printed];
				std::printf("  %s  %-24s %4d dBm  %s\n", FormatAddress(device.address).c_str(),
				            device.name.empty() ? "(no name)" : device.name.c_str(), device.rssi,
				            RolesText(device.roles).c_str());
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		manager.StopScan();
		if (manager.GetDiscoveredDevices().empty())
		{
			std::printf("No fitness devices found.\n");
		}
		if (!anyDevice)
		{
			return 0;
		}
	}

	// ---- connect -------------------------------------------------------------------------------------------
	auto assign = [&manager, &now](DeviceRole role, const std::string& addressText, const std::string& name)
	{
		BleAddress address = 0;
		if (!ParseAddress(addressText, address))
		{
			std::fprintf(stderr, "Invalid address '%s' (expected AA:BB:CC:DD:EE:FF)\n", addressText.c_str());
			return false;
		}
		std::printf("Connecting %s %s\n", ToString(role), addressText.c_str());
		return manager.AssignDevice(role, address, name, now());
	};

	if (options.useRegistry && manager.LoadRegistry())
	{
		manager.ConnectRememberedDevices(now());
	}
	if (options.autoConnect)
	{
		for (DeviceRole role : {DeviceRole::TRAINER, DeviceRole::HEART_RATE, DeviceRole::POWER_METER})
		{
			for (const DiscoveredDevice& device : manager.GetDiscoveredDevices())
			{
				// Primary role only, so a trainer is not also grabbed as the power meter (one link per device).
				if (!device.roles.empty() && device.roles.front() == role && !manager.GetDevice(role))
				{
					assign(role, FormatAddress(device.address), device.name);
				}
			}
		}
	}
	bool ok = true;
	if (options.trainerAddress)
	{
		ok = assign(DeviceRole::TRAINER, *options.trainerAddress, "trainer") && ok;
	}
	if (options.heartRateAddress)
	{
		ok = assign(DeviceRole::HEART_RATE, *options.heartRateAddress, "heart rate") && ok;
	}
	if (options.powerMeterAddress)
	{
		ok = assign(DeviceRole::POWER_METER, *options.powerMeterAddress, "power meter") && ok;
	}
	if (!ok)
	{
		manager.Shutdown();
		return 1;
	}

	// ---- live loop -----------------------------------------------------------------------------------------
	bool targetSent        = false;
	double nextStatus      = 0.0;
	double nextSimulation  = 0.0;
	const double loopStart = now();
	while (!g_stopRequested && (!options.durationSeconds || now() - loopStart < *options.durationSeconds))
	{
		const double time = now();
		if (options.simulate && time >= nextSimulation)
		{
			simulation.Tick(time);
			nextSimulation = time + 0.5;
		}
		manager.Update(time);
		for (const DeviceEvent& event : manager.TakeEvents())
		{
			PrintEvent(event, time);
		}

		Trainer* trainer = manager.GetTrainer();
		if (trainer && !targetSent && trainer->GetControlState() == ControlState::GRANTED)
		{
			targetSent = true;
			std::optional<CommandResult> result;
			if (options.targetPowerWatts)
			{
				result = trainer->SetTargetPower(*options.targetPowerWatts);
				std::printf("Set Target Power %d W -> %s (applying %.0f W)\n", *options.targetPowerWatts,
				            ToString(result->status), result->appliedValue);
			}
			else if (options.gradePercent)
			{
				result = trainer->SetSimulation(SimulationTarget{*options.gradePercent});
				std::printf("Set grade %.1f %% -> %s\n", *options.gradePercent, ToString(result->status));
			}
			else if (options.manualResistance)
			{
				result = trainer->SetManualResistance(*options.manualResistance);
				std::printf("Set manual resistance %.2f -> %s (applied %.2f)\n", *options.manualResistance,
				            ToString(result->status), result->appliedValue);
			}
		}

		if (time >= nextStatus)
		{
			PrintStatus(manager, time);
			nextStatus = time + 1.0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::printf("Stopping: releasing trainer control and closing BLE links...\n");
	manager.Shutdown();
	for (const DeviceEvent& event : manager.TakeEvents())
	{
		PrintEvent(event, now());
	}
	std::printf("Done.\n");
	return 0;
}
