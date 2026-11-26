#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "Engine/Fitness/Protocol/BleUuid.h"
#include "Engine/Fitness/Protocol/Bytes.h"

namespace Struktur::Fitness
{
// 48-bit Bluetooth device address in the low bits (the form Windows' BluetoothLEDevice uses natively).
using BleAddress = uint64_t;

std::string FormatAddress(BleAddress address);  // "AA:BB:CC:DD:EE:FF"
bool ParseAddress(const std::string& text, BleAddress& out);

enum class BleStatus : uint8_t
{
	SUCCESS,
	NOT_AVAILABLE,   // no adapter / backend not supported on this platform
	NOT_FOUND,       // unknown device, or service/characteristic not discovered
	UNREACHABLE,     // device out of range or connection failed
	PROTOCOL_ERROR,  // ATT error from the peripheral
	ACCESS_DENIED,
	NOT_CONNECTED,
	UNKNOWN_ERROR,
};

const char* ToString(BleStatus status);

// GATT characteristic property bits (Bluetooth Core Spec Vol 3 Part G 3.3.1.1).
namespace CharacteristicProperty
{
constexpr uint32_t READ                   = 0x02;
constexpr uint32_t WRITE_WITHOUT_RESPONSE = 0x04;
constexpr uint32_t WRITE                  = 0x08;
constexpr uint32_t NOTIFY                 = 0x10;
constexpr uint32_t INDICATE               = 0x20;
}  // namespace CharacteristicProperty

struct BleAdvertisement
{
	BleAddress address = 0;
	std::string name;
	int16_t rssi = 0;
	std::vector<BleUuid> serviceUuids;
};

struct BleCharacteristicInfo
{
	BleUuid uuid;
	uint32_t properties = 0;
};

struct BleServiceInfo
{
	BleUuid uuid;
	std::vector<BleCharacteristicInfo> characteristics;
};

struct BleCharacteristicRef
{
	BleUuid service;
	BleUuid characteristic;

	bool operator==(const BleCharacteristicRef& other) const  = default;
	auto operator<=>(const BleCharacteristicRef& other) const = default;
};

enum class SubscriptionKind : uint8_t
{
	NOTIFY,
	INDICATE,
};

// The minimal BLE central role the fitness devices need. One instance is shared by every device.
//
// Threading contract:
//  - Methods are called from the game thread.
//  - Callbacks may run on ANY thread (a backend worker, an OS thread pool), and may even run synchronously inside
//    the call that triggered them. They must be cheap, must not block, and must not call back into the central -
//    the device layer only ever pushes the result into a thread-safe queue and handles it on the game thread.
//  - After Disconnect(address) returns, no further callbacks for that address are delivered except ones already
//    in flight; the device layer filters those out by connection generation.
//
// Every async operation reports exactly one completion callback.
class IBleCentral
{
public:
	using AdvertisementCallback = std::function<void(const BleAdvertisement&)>;
	using StatusCallback        = std::function<void(BleStatus)>;
	using DiscoverCallback      = std::function<void(BleStatus, std::vector<BleServiceInfo>)>;
	using ReadCallback          = std::function<void(BleStatus, Bytes)>;
	using ValueCallback         = std::function<void(Bytes)>;
	using DisconnectCallback    = std::function<void()>;

	virtual ~IBleCentral() = default;

	// False when there is no usable adapter (or no backend for this platform). Everything still "works" - each
	// operation just completes with NOT_AVAILABLE - so callers never need a separate code path.
	virtual bool IsAvailable() const = 0;

	// Reports advertisements that list at least one of serviceFilter (empty filter = everything). Repeated
	// advertisements from one device are reported repeatedly; the scanner de-duplicates.
	virtual void StartScan(std::vector<BleUuid> serviceFilter, AdvertisementCallback onAdvertisement) = 0;
	virtual void StopScan()                                                                           = 0;

	// onDisconnected fires once if an established link drops unexpectedly (not after Disconnect()).
	virtual void Connect(BleAddress address, DisconnectCallback onDisconnected, StatusCallback onDone) = 0;
	// Must release every OS resource for the link. Safe to call for an address that is not connected.
	virtual void Disconnect(BleAddress address) = 0;

	virtual void DiscoverServices(BleAddress address, DiscoverCallback onDone)                             = 0;
	virtual void Read(BleAddress address, const BleCharacteristicRef& characteristic, ReadCallback onDone) = 0;
	// Enables notifications/indications (writes the CCCD) and routes each value to onValue.
	virtual void Subscribe(BleAddress address, const BleCharacteristicRef& characteristic, SubscriptionKind kind,
	                       ValueCallback onValue, StatusCallback onDone) = 0;
	virtual void Write(BleAddress address, const BleCharacteristicRef& characteristic, Bytes value, bool withResponse,
	                   StatusCallback onDone)                            = 0;
};
}  // namespace Struktur::Fitness
