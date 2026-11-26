#include "WinRtBleCentral.h"

#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <future>
#include <map>
#include <mutex>
#include <thread>

#include "Debug/Assertions.h"

using winrt::Windows::Devices::Bluetooth::BluetoothAdapter;
using winrt::Windows::Devices::Bluetooth::BluetoothCacheMode;
using winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus;
using winrt::Windows::Devices::Bluetooth::BluetoothLEDevice;
using winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs;
using winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher;
using winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEScanningMode;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs;
using winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteOption;
using winrt::Windows::Storage::Streams::DataReader;
using winrt::Windows::Storage::Streams::DataWriter;
using winrt::Windows::Storage::Streams::IBuffer;

namespace Struktur::Fitness
{
static BleUuid FromGuid(const winrt::guid& guid)
{
	BleUuid uuid;
	uuid.bytes[0] = static_cast<uint8_t>(guid.Data1 >> 24);
	uuid.bytes[1] = static_cast<uint8_t>(guid.Data1 >> 16);
	uuid.bytes[2] = static_cast<uint8_t>(guid.Data1 >> 8);
	uuid.bytes[3] = static_cast<uint8_t>(guid.Data1);
	uuid.bytes[4] = static_cast<uint8_t>(guid.Data2 >> 8);
	uuid.bytes[5] = static_cast<uint8_t>(guid.Data2);
	uuid.bytes[6] = static_cast<uint8_t>(guid.Data3 >> 8);
	uuid.bytes[7] = static_cast<uint8_t>(guid.Data3);
	std::memcpy(uuid.bytes.data() + 8, guid.Data4, 8);
	return uuid;
}

static Bytes ToBytes(const IBuffer& buffer)
{
	if (!buffer)
	{
		return {};
	}
	DataReader reader = DataReader::FromBuffer(buffer);
	Bytes bytes(reader.UnconsumedBufferLength());
	if (!bytes.empty())
	{
		reader.ReadBytes(bytes);
	}
	return bytes;
}

static IBuffer ToBuffer(const Bytes& bytes)
{
	DataWriter writer;
	writer.WriteBytes(bytes);
	return writer.DetachBuffer();
}

static BleStatus ToStatus(GattCommunicationStatus status)
{
	switch (status)
	{
		case GattCommunicationStatus::Success:
			return BleStatus::SUCCESS;
		case GattCommunicationStatus::Unreachable:
			return BleStatus::UNREACHABLE;
		case GattCommunicationStatus::ProtocolError:
			return BleStatus::PROTOCOL_ERROR;
		case GattCommunicationStatus::AccessDenied:
			return BleStatus::ACCESS_DENIED;
	}
	return BleStatus::UNKNOWN_ERROR;
}

struct WinRtBleCentral::Impl
{
	struct Subscription
	{
		GattCharacteristic characteristic{nullptr};
		winrt::event_token token{};
	};

	struct Link
	{
		BluetoothLEDevice device{nullptr};
		GattSession session{nullptr};
		winrt::event_token statusToken{};
		std::vector<GattDeviceService> services;
		std::map<BleCharacteristicRef, GattCharacteristic> characteristics;
		std::map<BleCharacteristicRef, Subscription> subscriptions;
		DisconnectCallback onDisconnected;
		bool closing = false;  // guarded by linkMutex
	};

	// --- worker ------------------------------------------------------------------------------------------------
	std::thread worker;
	std::mutex jobMutex;
	std::condition_variable jobCondition;
	std::deque<std::function<void()>> jobs;
	bool stopping = false;
	std::atomic<bool> available{false};

	// --- scanning (watcher touched only on the worker; the rest under scanMutex) -----------------------------
	BluetoothLEAdvertisementWatcher watcher{nullptr};
	winrt::event_token receivedToken{};
	std::mutex scanMutex;
	std::vector<BleUuid> scanFilter;
	AdvertisementCallback onAdvertisement;
	std::map<BleAddress, BleAdvertisement> seenAdvertisements;

	// --- links (winrt objects used on the worker; the map itself and Link::closing under linkMutex) ---------
	std::mutex linkMutex;
	std::map<BleAddress, std::shared_ptr<Link>> links;

	Impl()
	{
		std::promise<void> started;
		std::future<void> startedFuture = started.get_future();
		worker                          = std::thread([this, &started]() { WorkerMain(started); });
		startedFuture.wait();
	}

	~Impl()
	{
		{
			std::lock_guard<std::mutex> lock(jobMutex);
			stopping = true;
		}
		jobCondition.notify_all();
		if (worker.joinable())
		{
			worker.join();
		}
	}

	void Post(std::function<void()> job)
	{
		{
			std::lock_guard<std::mutex> lock(jobMutex);
			jobs.push_back(std::move(job));
		}
		jobCondition.notify_one();
	}

	void WorkerMain(std::promise<void>& started)
	{
		winrt::init_apartment(winrt::apartment_type::multi_threaded);
		try
		{
			BluetoothAdapter adapter = BluetoothAdapter::GetDefaultAsync().get();
			available                = adapter && adapter.IsLowEnergySupported() && adapter.IsCentralRoleSupported();
			if (!available)
			{
				DEBUG_WARNING("No Bluetooth LE adapter with central role support found");
			}
		}
		catch (const winrt::hresult_error& error)
		{
			DEBUG_WARNING("Bluetooth adapter query failed: %s", winrt::to_string(error.message()).c_str());
			available = false;
		}
		started.set_value();

		while (true)
		{
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(jobMutex);
				jobCondition.wait(lock, [this]() { return stopping || !jobs.empty(); });
				// Drain queued jobs (e.g. Disconnects issued during shutdown) before honouring stop.
				if (jobs.empty())
				{
					break;
				}
				job = std::move(jobs.front());
				jobs.pop_front();
			}
			RunGuarded(job);
		}

		RunGuarded([this]() { StopWatcher(); });
		std::map<BleAddress, std::shared_ptr<Link>> remaining;
		{
			std::lock_guard<std::mutex> lock(linkMutex);
			remaining.swap(links);
		}
		for (auto& [address, link] : remaining)
		{
			RunGuarded([&link]() { CloseLink(*link); });
		}
		winrt::uninit_apartment();
	}

	static void RunGuarded(const std::function<void()>& job)
	{
		try
		{
			job();
		}
		catch (const winrt::hresult_error& error)
		{
			DEBUG_ERROR("WinRT BLE operation threw: %s", winrt::to_string(error.message()).c_str());
		}
		catch (const std::exception& error)
		{
			DEBUG_ERROR("BLE operation threw: %s", error.what());
		}
	}

	std::shared_ptr<Link> FindLink(BleAddress address)
	{
		std::lock_guard<std::mutex> lock(linkMutex);
		auto found = links.find(address);
		return found == links.end() || found->second->closing ? nullptr : found->second;
	}

	// --- scanning ------------------------------------------------------------------------------------------

	void StartWatcher()
	{
		if (!watcher)
		{
			watcher = BluetoothLEAdvertisementWatcher();
			// Active scanning requests scan responses, which is where most trainers put their name.
			watcher.ScanningMode(BluetoothLEScanningMode::Active);
			receivedToken = watcher.Received(
			    [this](const BluetoothLEAdvertisementWatcher&, const BluetoothLEAdvertisementReceivedEventArgs& args)
			    { OnAdvertisementReceived(args); });
		}
		watcher.Start();
	}

	void StopWatcher()
	{
		if (watcher)
		{
			watcher.Stop();
		}
	}

	void OnAdvertisementReceived(const BluetoothLEAdvertisementReceivedEventArgs& args)
	{
		BleAdvertisement snapshot;
		AdvertisementCallback callback;
		{
			std::lock_guard<std::mutex> lock(scanMutex);
			if (!onAdvertisement)
			{
				return;
			}
			// Names and service lists often arrive in different packets (advert vs scan response): merge per address.
			BleAdvertisement& entry = seenAdvertisements[args.BluetoothAddress()];
			entry.address           = args.BluetoothAddress();
			entry.rssi              = args.RawSignalStrengthInDBm();
			const std::string name  = winrt::to_string(args.Advertisement().LocalName());
			if (!name.empty())
			{
				entry.name = name;
			}
			for (const winrt::guid& guid : args.Advertisement().ServiceUuids())
			{
				const BleUuid uuid = FromGuid(guid);
				if (std::find(entry.serviceUuids.begin(), entry.serviceUuids.end(), uuid) == entry.serviceUuids.end())
				{
					entry.serviceUuids.push_back(uuid);
				}
			}

			bool matches = scanFilter.empty();
			for (const BleUuid& uuid : entry.serviceUuids)
			{
				matches = matches || std::find(scanFilter.begin(), scanFilter.end(), uuid) != scanFilter.end();
			}
			if (!matches)
			{
				return;
			}
			snapshot = entry;
			callback = onAdvertisement;
		}
		callback(snapshot);
	}

	// --- links ---------------------------------------------------------------------------------------------

	void OnConnectionStatusChanged(BleAddress address, const BluetoothLEDevice& sender)
	{
		if (sender.ConnectionStatus() != BluetoothConnectionStatus::Disconnected)
		{
			return;
		}
		DisconnectCallback callback;
		{
			std::lock_guard<std::mutex> lock(linkMutex);
			auto found = links.find(address);
			if (found == links.end() || found->second->closing)
			{
				return;
			}
			found->second->closing = true;
			callback               = std::move(found->second->onDisconnected);
		}
		DEBUG_WARNING("BLE link to %s lost", FormatAddress(address).c_str());
		Post([this, address]() { RemoveLink(address); });
		if (callback)
		{
			callback();
		}
	}

	void ConnectJob(BleAddress address, DisconnectCallback onDisconnected, const StatusCallback& onDone)
	{
		if (!available)
		{
			onDone(BleStatus::NOT_AVAILABLE);
			return;
		}
		RemoveLink(address);

		BluetoothLEDevice device = BluetoothLEDevice::FromBluetoothAddressAsync(address).get();
		if (!device)
		{
			onDone(BleStatus::NOT_FOUND);
			return;
		}

		auto link            = std::make_shared<Link>();
		link->device         = device;
		link->onDisconnected = std::move(onDisconnected);
		try
		{
			// Without a GattSession that maintains the connection, Windows may drop an idle link on its own.
			link->session = GattSession::FromDeviceIdAsync(device.BluetoothDeviceId()).get();
			link->session.MaintainConnection(true);
		}
		catch (const winrt::hresult_error& error)
		{
			DEBUG_WARNING("GattSession unavailable for %s: %s", FormatAddress(address).c_str(),
			              winrt::to_string(error.message()).c_str());
		}

		// The first uncached GATT request is what actually brings the radio link up.
		auto result = device.GetGattServicesAsync(BluetoothCacheMode::Uncached).get();
		if (result.Status() != GattCommunicationStatus::Success)
		{
			CloseLink(*link);
			onDone(ToStatus(result.Status()));
			return;
		}
		for (const GattDeviceService& service : result.Services())
		{
			link->services.push_back(service);
		}

		link->statusToken = device.ConnectionStatusChanged(
		    [this, address](const BluetoothLEDevice& sender, const winrt::Windows::Foundation::IInspectable&)
		    { OnConnectionStatusChanged(address, sender); });
		{
			std::lock_guard<std::mutex> lock(linkMutex);
			links[address] = link;
		}
		onDone(BleStatus::SUCCESS);
	}

	void DiscoverJob(BleAddress address, const DiscoverCallback& onDone)
	{
		std::shared_ptr<Link> link = FindLink(address);
		if (!link)
		{
			onDone(BleStatus::NOT_CONNECTED, {});
			return;
		}
		std::vector<BleServiceInfo> services;
		link->characteristics.clear();
		for (const GattDeviceService& service : link->services)
		{
			BleServiceInfo serviceInfo;
			serviceInfo.uuid = FromGuid(service.Uuid());
			auto result      = service.GetCharacteristicsAsync(BluetoothCacheMode::Uncached).get();
			if (result.Status() != GattCommunicationStatus::Success)
			{
				// Some services (e.g. HID) refuse access; skip them rather than failing the whole device.
				DEBUG_WARNING("Characteristic discovery failed for service %s on %s",
				              serviceInfo.uuid.ToString().c_str(), FormatAddress(address).c_str());
				services.push_back(serviceInfo);
				continue;
			}
			for (const GattCharacteristic& characteristic : result.Characteristics())
			{
				const BleUuid uuid = FromGuid(characteristic.Uuid());
				serviceInfo.characteristics.push_back(
				    BleCharacteristicInfo{uuid, static_cast<uint32_t>(characteristic.CharacteristicProperties())});
				link->characteristics.insert_or_assign(BleCharacteristicRef{serviceInfo.uuid, uuid}, characteristic);
			}
			services.push_back(std::move(serviceInfo));
		}
		onDone(BleStatus::SUCCESS, std::move(services));
	}

	GattCharacteristic FindCharacteristic(const std::shared_ptr<Link>& link, const BleCharacteristicRef& reference)
	{
		auto found = link->characteristics.find(reference);
		return found == link->characteristics.end() ? GattCharacteristic{nullptr} : found->second;
	}

	void ReadJob(BleAddress address, const BleCharacteristicRef& reference, const ReadCallback& onDone)
	{
		std::shared_ptr<Link> link = FindLink(address);
		if (!link)
		{
			onDone(BleStatus::NOT_CONNECTED, {});
			return;
		}
		GattCharacteristic characteristic = FindCharacteristic(link, reference);
		if (!characteristic)
		{
			onDone(BleStatus::NOT_FOUND, {});
			return;
		}
		auto result = characteristic.ReadValueAsync(BluetoothCacheMode::Uncached).get();
		if (result.Status() != GattCommunicationStatus::Success)
		{
			onDone(ToStatus(result.Status()), {});
			return;
		}
		onDone(BleStatus::SUCCESS, ToBytes(result.Value()));
	}

	void SubscribeJob(BleAddress address, const BleCharacteristicRef& reference, SubscriptionKind kind,
	                  ValueCallback onValue, const StatusCallback& onDone)
	{
		std::shared_ptr<Link> link = FindLink(address);
		if (!link)
		{
			onDone(BleStatus::NOT_CONNECTED);
			return;
		}
		GattCharacteristic characteristic = FindCharacteristic(link, reference);
		if (!characteristic)
		{
			onDone(BleStatus::NOT_FOUND);
			return;
		}

		RemoveSubscription(*link, reference, false);
		Subscription subscription;
		subscription.characteristic = characteristic;
		subscription.token          = characteristic.ValueChanged(
            [onValue = std::move(onValue)](const GattCharacteristic&, const GattValueChangedEventArgs& args)
            { onValue(ToBytes(args.CharacteristicValue())); });

		const auto descriptorValue = kind == SubscriptionKind::NOTIFY
		                                 ? GattClientCharacteristicConfigurationDescriptorValue::Notify
		                                 : GattClientCharacteristicConfigurationDescriptorValue::Indicate;
		const GattCommunicationStatus status =
		    characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(descriptorValue).get();
		if (status != GattCommunicationStatus::Success)
		{
			characteristic.ValueChanged(subscription.token);
			onDone(ToStatus(status));
			return;
		}
		link->subscriptions[reference] = subscription;
		onDone(BleStatus::SUCCESS);
	}

	void WriteJob(BleAddress address, const BleCharacteristicRef& reference, const Bytes& value, bool withResponse,
	              const StatusCallback& onDone)
	{
		std::shared_ptr<Link> link = FindLink(address);
		if (!link)
		{
			onDone(BleStatus::NOT_CONNECTED);
			return;
		}
		GattCharacteristic characteristic = FindCharacteristic(link, reference);
		if (!characteristic)
		{
			onDone(BleStatus::NOT_FOUND);
			return;
		}
		auto result =
		    characteristic
		        .WriteValueWithResultAsync(ToBuffer(value), withResponse ? GattWriteOption::WriteWithResponse
		                                                                 : GattWriteOption::WriteWithoutResponse)
		        .get();
		if (result.Status() == GattCommunicationStatus::ProtocolError && result.ProtocolError())
		{
			DEBUG_WARNING("GATT write to %s failed with ATT error 0x%02X", reference.characteristic.ToString().c_str(),
			              static_cast<unsigned>(result.ProtocolError().Value()));
		}
		onDone(ToStatus(result.Status()));
	}

	static void RemoveSubscription(Link& link, const BleCharacteristicRef& reference, bool writeDescriptor)
	{
		auto found = link.subscriptions.find(reference);
		if (found == link.subscriptions.end())
		{
			return;
		}
		Subscription subscription = found->second;
		link.subscriptions.erase(found);
		subscription.characteristic.ValueChanged(subscription.token);
		if (writeDescriptor)
		{
			try
			{
				subscription.characteristic
				    .WriteClientCharacteristicConfigurationDescriptorAsync(
				        GattClientCharacteristicConfigurationDescriptorValue::None)
				    .get();
			}
			catch (const winrt::hresult_error&)
			{
				// Best effort - the link may already be gone.
			}
		}
	}

	// Releases every handle Windows needs to see closed before it drops the radio link. A trainer that allows only
	// one connection (Elite Direto) stays blocked for other apps until this has run.
	static void CloseLink(Link& link)
	{
		const bool connected = link.device && link.device.ConnectionStatus() == BluetoothConnectionStatus::Connected;
		while (!link.subscriptions.empty())
		{
			RemoveSubscription(link, link.subscriptions.begin()->first, connected);
		}
		link.characteristics.clear();
		if (link.device && link.statusToken.value != 0)
		{
			link.device.ConnectionStatusChanged(link.statusToken);
			link.statusToken = {};
		}
		if (link.session)
		{
			link.session.MaintainConnection(false);
			link.session.Close();
			link.session = nullptr;
		}
		for (GattDeviceService& service : link.services)
		{
			service.Close();
		}
		link.services.clear();
		if (link.device)
		{
			link.device.Close();
			link.device = nullptr;
		}
	}

	void RemoveLink(BleAddress address)
	{
		std::shared_ptr<Link> link;
		{
			std::lock_guard<std::mutex> lock(linkMutex);
			auto found = links.find(address);
			if (found == links.end())
			{
				return;
			}
			link = found->second;
			links.erase(found);
		}
		CloseLink(*link);
	}
};

WinRtBleCentral::WinRtBleCentral()
    : m_impl(std::make_unique<Impl>())
{
}

WinRtBleCentral::~WinRtBleCentral()
{
	StopScan();
	std::vector<BleAddress> addresses;
	{
		std::lock_guard<std::mutex> lock(m_impl->linkMutex);
		for (const auto& [address, link] : m_impl->links)
		{
			addresses.push_back(address);
		}
	}
	for (BleAddress address : addresses)
	{
		Disconnect(address);
	}
	m_impl.reset();  // joins the worker after it has drained the Disconnect jobs
}

bool WinRtBleCentral::IsAvailable() const
{
	return m_impl->available;
}

void WinRtBleCentral::StartScan(std::vector<BleUuid> serviceFilter, AdvertisementCallback onAdvertisement)
{
	{
		std::lock_guard<std::mutex> lock(m_impl->scanMutex);
		m_impl->scanFilter      = std::move(serviceFilter);
		m_impl->onAdvertisement = std::move(onAdvertisement);
		m_impl->seenAdvertisements.clear();
	}
	if (!m_impl->available)
	{
		return;
	}
	Impl* impl = m_impl.get();
	m_impl->Post([impl]() { impl->StartWatcher(); });
}

void WinRtBleCentral::StopScan()
{
	{
		std::lock_guard<std::mutex> lock(m_impl->scanMutex);
		m_impl->onAdvertisement = nullptr;
	}
	Impl* impl = m_impl.get();
	m_impl->Post([impl]() { impl->StopWatcher(); });
}

void WinRtBleCentral::Connect(BleAddress address, DisconnectCallback onDisconnected, StatusCallback onDone)
{
	Impl* impl = m_impl.get();
	m_impl->Post(
	    [impl, address, onDisconnected = std::move(onDisconnected), onDone = std::move(onDone)]() mutable
	    {
		    try
		    {
			    impl->ConnectJob(address, std::move(onDisconnected), onDone);
		    }
		    catch (const winrt::hresult_error& error)
		    {
			    DEBUG_ERROR("Connect to %s threw: %s", FormatAddress(address).c_str(),
			                winrt::to_string(error.message()).c_str());
			    impl->RemoveLink(address);
			    onDone(BleStatus::UNKNOWN_ERROR);
		    }
	    });
}

void WinRtBleCentral::Disconnect(BleAddress address)
{
	{
		// Mark closing now so a racing connection-status event cannot fire onDisconnected after this returns.
		std::lock_guard<std::mutex> lock(m_impl->linkMutex);
		auto found = m_impl->links.find(address);
		if (found != m_impl->links.end())
		{
			found->second->closing = true;
		}
	}
	Impl* impl = m_impl.get();
	m_impl->Post([impl, address]() { impl->RemoveLink(address); });
}

void WinRtBleCentral::DiscoverServices(BleAddress address, DiscoverCallback onDone)
{
	Impl* impl = m_impl.get();
	m_impl->Post(
	    [impl, address, onDone = std::move(onDone)]()
	    {
		    try
		    {
			    impl->DiscoverJob(address, onDone);
		    }
		    catch (const winrt::hresult_error& error)
		    {
			    DEBUG_ERROR("Discovery on %s threw: %s", FormatAddress(address).c_str(),
			                winrt::to_string(error.message()).c_str());
			    onDone(BleStatus::UNKNOWN_ERROR, {});
		    }
	    });
}

void WinRtBleCentral::Read(BleAddress address, const BleCharacteristicRef& characteristic, ReadCallback onDone)
{
	Impl* impl = m_impl.get();
	m_impl->Post(
	    [impl, address, characteristic, onDone = std::move(onDone)]()
	    {
		    try
		    {
			    impl->ReadJob(address, characteristic, onDone);
		    }
		    catch (const winrt::hresult_error& error)
		    {
			    DEBUG_ERROR("Read threw: %s", winrt::to_string(error.message()).c_str());
			    onDone(BleStatus::UNKNOWN_ERROR, {});
		    }
	    });
}

void WinRtBleCentral::Subscribe(BleAddress address, const BleCharacteristicRef& characteristic, SubscriptionKind kind,
                                ValueCallback onValue, StatusCallback onDone)
{
	Impl* impl = m_impl.get();
	m_impl->Post(
	    [impl, address, characteristic, kind, onValue = std::move(onValue), onDone = std::move(onDone)]() mutable
	    {
		    try
		    {
			    impl->SubscribeJob(address, characteristic, kind, std::move(onValue), onDone);
		    }
		    catch (const winrt::hresult_error& error)
		    {
			    DEBUG_ERROR("Subscribe threw: %s", winrt::to_string(error.message()).c_str());
			    onDone(BleStatus::UNKNOWN_ERROR);
		    }
	    });
}

void WinRtBleCentral::Write(BleAddress address, const BleCharacteristicRef& characteristic, Bytes value,
                            bool withResponse, StatusCallback onDone)
{
	Impl* impl = m_impl.get();
	m_impl->Post(
	    [impl, address, characteristic, value = std::move(value), withResponse, onDone = std::move(onDone)]()
	    {
		    try
		    {
			    impl->WriteJob(address, characteristic, value, withResponse, onDone);
		    }
		    catch (const winrt::hresult_error& error)
		    {
			    DEBUG_ERROR("Write threw: %s", winrt::to_string(error.message()).c_str());
			    onDone(BleStatus::UNKNOWN_ERROR);
		    }
	    });
}
}  // namespace Struktur::Fitness
