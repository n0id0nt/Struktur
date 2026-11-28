#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Struktur::Fitness
{
// A 128-bit Bluetooth UUID, stored in canonical (big-endian, as printed) byte order. 16-bit SIG-assigned UUIDs
// expand onto the Bluetooth Base UUID 0000xxxx-0000-1000-8000-00805F9B34FB.
struct BleUuid
{
	std::array<uint8_t, 16> bytes{};

	static constexpr BleUuid FromShort(uint16_t shortUuid)
	{
		BleUuid uuid;
		uuid.bytes = {0x00,
		              0x00,
		              static_cast<uint8_t>(shortUuid >> 8),
		              static_cast<uint8_t>(shortUuid & 0xFF),
		              0x00,
		              0x00,
		              0x10,
		              0x00,
		              0x80,
		              0x00,
		              0x00,
		              0x80,
		              0x5F,
		              0x9B,
		              0x34,
		              0xFB};
		return uuid;
	}

	// The 16-bit form when this UUID sits on the Base UUID, otherwise nullopt.
	std::optional<uint16_t> GetShort() const;

	// Lower-case "0000180d-0000-1000-8000-00805f9b34fb" form.
	std::string ToString() const;

	// Accepts the 36-character dashed form or a 4-hex-digit short form ("180D").
	static std::optional<BleUuid> Parse(std::string_view text);

	bool operator==(const BleUuid& other) const  = default;
	auto operator<=>(const BleUuid& other) const = default;
};

// SIG-assigned numbers used by this module (Bluetooth Assigned Numbers, "16-bit UUIDs").
namespace Uuids
{
constexpr uint16_t HEART_RATE_SERVICE      = 0x180D;
constexpr uint16_t CYCLING_POWER_SERVICE   = 0x1818;
constexpr uint16_t FITNESS_MACHINE_SERVICE = 0x1826;

constexpr uint16_t HEART_RATE_MEASUREMENT    = 0x2A37;
constexpr uint16_t CYCLING_POWER_MEASUREMENT = 0x2A63;
constexpr uint16_t CYCLING_POWER_FEATURE     = 0x2A65;

constexpr uint16_t FTMS_FEATURE              = 0x2ACC;
constexpr uint16_t FTMS_INDOOR_BIKE_DATA     = 0x2AD2;
constexpr uint16_t FTMS_SUPPORTED_INCLINATION = 0x2AD5;
constexpr uint16_t FTMS_SUPPORTED_RESISTANCE = 0x2AD6;
constexpr uint16_t FTMS_SUPPORTED_POWER      = 0x2AD8;
constexpr uint16_t FTMS_CONTROL_POINT        = 0x2AD9;
constexpr uint16_t FTMS_MACHINE_STATUS       = 0x2ADA;
}  // namespace Uuids
}  // namespace Struktur::Fitness
