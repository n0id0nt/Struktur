#include "FitWriter.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>

namespace Struktur::Ride
{
// --- FIT constants (FIT protocol / profile) ------------------------------------------------------------------------

static constexpr int64_t FIT_EPOCH_UNIX = 631065600;  // 1989-12-31 00:00:00 UTC

namespace BaseType
{
constexpr uint8_t ENUM    = 0x00;
constexpr uint8_t UINT8   = 0x02;
constexpr uint8_t UINT16  = 0x84;
constexpr uint8_t UINT32  = 0x86;
constexpr uint8_t UINT32Z = 0x8C;
}  // namespace BaseType

namespace GlobalMessage
{
constexpr uint16_t FILE_ID  = 0;
constexpr uint16_t SESSION  = 18;
constexpr uint16_t LAP      = 19;
constexpr uint16_t RECORD   = 20;
constexpr uint16_t EVENT    = 21;
constexpr uint16_t ACTIVITY = 34;
}  // namespace GlobalMessage

namespace LocalMessage
{
constexpr uint8_t FILE_ID  = 0;
constexpr uint8_t EVENT    = 1;
constexpr uint8_t RECORD   = 2;
constexpr uint8_t LAP      = 3;
constexpr uint8_t SESSION  = 4;
constexpr uint8_t ACTIVITY = 5;
}  // namespace LocalMessage

constexpr uint8_t FILE_TYPE_ACTIVITY           = 4;
constexpr uint16_t MANUFACTURER_DEVELOPMENT    = 255;
constexpr uint8_t EVENT_TIMER                  = 0;
constexpr uint8_t EVENT_SESSION                = 8;
constexpr uint8_t EVENT_LAP                    = 9;
constexpr uint8_t EVENT_ACTIVITY               = 26;
constexpr uint8_t EVENT_TYPE_START             = 0;
constexpr uint8_t EVENT_TYPE_STOP              = 1;
constexpr uint8_t EVENT_TYPE_STOP_ALL          = 4;
constexpr uint8_t SPORT_CYCLING                = 2;
constexpr uint8_t SUB_SPORT_INDOOR_CYCLING     = 6;
constexpr uint8_t LAP_TRIGGER_MANUAL           = 0;
constexpr uint8_t LAP_TRIGGER_SESSION_END      = 7;
constexpr uint8_t SESSION_TRIGGER_ACTIVITY_END = 0;
constexpr uint8_t ACTIVITY_TYPE_MANUAL         = 0;

// --- encoding helpers --------------------------------------------------------------------------------------------

struct FieldDefinition
{
	uint8_t number;
	uint8_t size;
	uint8_t baseType;
};

class FitBuffer
{
public:
	void Definition(uint8_t localType, uint16_t globalNumber, const std::vector<FieldDefinition>& fields)
	{
		m_bytes.push_back((uint8_t)(0x40 | (localType & 0x0F)));
		m_bytes.push_back(0);  // reserved
		m_bytes.push_back(0);  // little-endian
		U16(globalNumber);
		m_bytes.push_back((uint8_t)fields.size());
		for (const FieldDefinition& field : fields)
		{
			m_bytes.push_back(field.number);
			m_bytes.push_back(field.size);
			m_bytes.push_back(field.baseType);
		}
	}

	void DataHeader(uint8_t localType)
	{
		m_bytes.push_back(localType & 0x0F);
	}

	void U8(uint8_t value)
	{
		m_bytes.push_back(value);
	}

	void U16(uint16_t value)
	{
		m_bytes.push_back((uint8_t)(value & 0xFF));
		m_bytes.push_back((uint8_t)(value >> 8));
	}

	void U32(uint32_t value)
	{
		for (int i = 0; i < 4; ++i)
		{
			m_bytes.push_back((uint8_t)((value >> (8 * i)) & 0xFF));
		}
	}

	std::vector<uint8_t>& Bytes()
	{
		return m_bytes;
	}

private:
	std::vector<uint8_t> m_bytes;
};

// Invalid ("no value") markers per base type.
static constexpr uint8_t INVALID_UINT8   = 0xFF;
static constexpr uint16_t INVALID_UINT16 = 0xFFFF;
static constexpr uint32_t INVALID_UINT32 = 0xFFFFFFFF;

static uint8_t ToU8(std::optional<int> value)
{
	return value && *value >= 0 && *value < INVALID_UINT8 ? (uint8_t)*value : INVALID_UINT8;
}

static uint16_t ToU16(std::optional<int> value)
{
	return value && *value >= 0 && *value < INVALID_UINT16 ? (uint16_t)*value : INVALID_UINT16;
}

static uint16_t ScaledU16(double value, double scale, double offset = 0.0)
{
	const double scaled = std::round((value + offset) * scale);
	return scaled >= 0.0 && scaled < INVALID_UINT16 ? (uint16_t)scaled : INVALID_UINT16;
}

static uint32_t ScaledU32(double value, double scale)
{
	const double scaled = std::round(value * scale);
	return scaled >= 0.0 && scaled < (double)INVALID_UINT32 ? (uint32_t)scaled : INVALID_UINT32;
}

uint16_t FitCrc(const uint8_t* data, size_t size, uint16_t crc)
{
	static constexpr uint16_t CRC_TABLE[16] = {0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	                                           0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400};
	for (size_t i = 0; i < size; ++i)
	{
		const uint8_t byte = data[i];
		uint16_t lookup    = CRC_TABLE[crc & 0xF];
		crc                = (uint16_t)((crc >> 4) & 0x0FFF);
		crc                = (uint16_t)(crc ^ lookup ^ CRC_TABLE[byte & 0xF]);
		lookup             = CRC_TABLE[crc & 0xF];
		crc                = (uint16_t)((crc >> 4) & 0x0FFF);
		crc                = (uint16_t)(crc ^ lookup ^ CRC_TABLE[(byte >> 4) & 0xF]);
	}
	return crc;
}

uint32_t ToFitTime(int64_t unixTime)
{
	return (uint32_t)std::max<int64_t>(0, unixTime - FIT_EPOCH_UNIX);
}

// --- message writers ---------------------------------------------------------------------------------------------

static void WriteEvent(FitBuffer& buffer, int64_t unixTime, uint8_t eventType)
{
	buffer.DataHeader(LocalMessage::EVENT);
	buffer.U32(ToFitTime(unixTime));
	buffer.U8(EVENT_TIMER);
	buffer.U8(eventType);
	buffer.U8(0);  // event_group
}

static void WriteRecord(FitBuffer& buffer, const RideSample& sample)
{
	buffer.DataHeader(LocalMessage::RECORD);
	buffer.U32(ToFitTime(sample.unixTime));
	buffer.U16(ScaledU16(sample.altitudeMeters, 5.0, 500.0));
	buffer.U8(ToU8(sample.heartRateBpm));
	buffer.U8(ToU8(sample.cadenceRpm));
	buffer.U32(ScaledU32(sample.distanceMeters, 100.0));
	buffer.U16(ScaledU16(sample.speedMetersPerSecond, 1000.0));
	buffer.U16(ToU16(sample.powerWatts));
}

static void WriteSummaryFields(FitBuffer& buffer, const RideSummary& summary)
{
	buffer.U32(ScaledU32(summary.elapsedSeconds, 1000.0));
	buffer.U32(ScaledU32(summary.timerSeconds, 1000.0));
	buffer.U32(ScaledU32(summary.distanceMeters, 100.0));
	buffer.U16(ScaledU16(summary.averageSpeed, 1000.0));
	buffer.U16(ScaledU16(summary.maximumSpeed, 1000.0));
	buffer.U16(ScaledU16(summary.ascentMeters, 1.0));
	buffer.U16(ToU16(summary.averagePower));
	buffer.U16(ToU16(summary.maximumPower));
	buffer.U8(ToU8(summary.averageHeartRate));
	buffer.U8(ToU8(summary.maximumHeartRate));
	buffer.U8(ToU8(summary.averageCadence));
}

std::vector<uint8_t> EncodeFitActivity(const RideLog& log, const FitOptions& options)
{
	FitBuffer buffer;
	const int64_t startUnix = log.GetStartUnix();
	const int64_t endUnix   = std::max(log.GetEndUnix(), startUnix);

	// file_id
	buffer.Definition(LocalMessage::FILE_ID, GlobalMessage::FILE_ID,
	                  {{0, 1, BaseType::ENUM},
	                   {1, 2, BaseType::UINT16},
	                   {2, 2, BaseType::UINT16},
	                   {3, 4, BaseType::UINT32Z},
	                   {4, 4, BaseType::UINT32}});
	buffer.DataHeader(LocalMessage::FILE_ID);
	buffer.U8(FILE_TYPE_ACTIVITY);
	buffer.U16(MANUFACTURER_DEVELOPMENT);
	buffer.U16(options.productId);
	buffer.U32(options.serialNumber == 0 ? 1 : options.serialNumber);
	buffer.U32(ToFitTime(startUnix));

	buffer.Definition(
	    LocalMessage::EVENT, GlobalMessage::EVENT,
	    {{253, 4, BaseType::UINT32}, {0, 1, BaseType::ENUM}, {1, 1, BaseType::ENUM}, {4, 1, BaseType::UINT8}});
	buffer.Definition(LocalMessage::RECORD, GlobalMessage::RECORD,
	                  {{253, 4, BaseType::UINT32},
	                   {2, 2, BaseType::UINT16},
	                   {3, 1, BaseType::UINT8},
	                   {4, 1, BaseType::UINT8},
	                   {5, 4, BaseType::UINT32},
	                   {6, 2, BaseType::UINT16},
	                   {7, 2, BaseType::UINT16}});

	// Records interleaved with the timer events (pauses), in time order.
	const std::vector<RideSample>& samples     = log.GetSamples();
	const std::vector<TimerEvent>& timerEvents = log.GetTimerEvents();
	size_t eventIndex                          = 0;
	for (const RideSample& sample : samples)
	{
		while (eventIndex < timerEvents.size() && timerEvents[eventIndex].unixTime <= sample.unixTime)
		{
			WriteEvent(buffer, timerEvents[eventIndex].unixTime,
			           timerEvents[eventIndex].started ? EVENT_TYPE_START : EVENT_TYPE_STOP_ALL);
			++eventIndex;
		}
		WriteRecord(buffer, sample);
	}
	for (; eventIndex < timerEvents.size(); ++eventIndex)
	{
		WriteEvent(buffer, timerEvents[eventIndex].unixTime,
		           timerEvents[eventIndex].started ? EVENT_TYPE_START : EVENT_TYPE_STOP_ALL);
	}
	WriteEvent(buffer, endUnix, EVENT_TYPE_STOP_ALL);

	// Laps
	buffer.Definition(LocalMessage::LAP, GlobalMessage::LAP,
	                  {{253, 4, BaseType::UINT32},
	                   {2, 4, BaseType::UINT32},
	                   {7, 4, BaseType::UINT32},
	                   {8, 4, BaseType::UINT32},
	                   {9, 4, BaseType::UINT32},
	                   {13, 2, BaseType::UINT16},
	                   {14, 2, BaseType::UINT16},
	                   {21, 2, BaseType::UINT16},
	                   {19, 2, BaseType::UINT16},
	                   {20, 2, BaseType::UINT16},
	                   {15, 1, BaseType::UINT8},
	                   {16, 1, BaseType::UINT8},
	                   {17, 1, BaseType::UINT8},
	                   {254, 2, BaseType::UINT16},
	                   {0, 1, BaseType::ENUM},
	                   {1, 1, BaseType::ENUM},
	                   {24, 1, BaseType::ENUM},
	                   {25, 1, BaseType::ENUM},
	                   {39, 1, BaseType::ENUM}});
	const std::vector<RideLap>& laps = log.GetLaps();
	uint16_t lapIndex                = 0;
	for (size_t i = 0; i < laps.size(); ++i)
	{
		const RideLap& lap = laps[i];
		const bool last    = i + 1 == laps.size();
		// Skip empty laps (e.g. a step skipped in under a second) except the last, so the file always has one.
		if (lap.endSample <= lap.firstSample && !last)
		{
			continue;
		}
		const RideSummary summary = log.SummarizeLap(lap);
		buffer.DataHeader(LocalMessage::LAP);
		buffer.U32(ToFitTime(std::max(lap.endUnix, lap.startUnix)));
		buffer.U32(ToFitTime(lap.startUnix));
		WriteSummaryFields(buffer, summary);
		buffer.U16(lapIndex++);
		buffer.U8(EVENT_LAP);
		buffer.U8(EVENT_TYPE_STOP);
		buffer.U8(last ? LAP_TRIGGER_SESSION_END : LAP_TRIGGER_MANUAL);
		buffer.U8(SPORT_CYCLING);
		buffer.U8(SUB_SPORT_INDOOR_CYCLING);
	}

	// Session
	const RideSummary ride = log.SummarizeRide();
	buffer.Definition(LocalMessage::SESSION, GlobalMessage::SESSION,
	                  {{253, 4, BaseType::UINT32}, {2, 4, BaseType::UINT32},  {7, 4, BaseType::UINT32},
	                   {8, 4, BaseType::UINT32},   {9, 4, BaseType::UINT32},  {14, 2, BaseType::UINT16},
	                   {15, 2, BaseType::UINT16},  {22, 2, BaseType::UINT16}, {20, 2, BaseType::UINT16},
	                   {21, 2, BaseType::UINT16},  {16, 1, BaseType::UINT8},  {17, 1, BaseType::UINT8},
	                   {18, 1, BaseType::UINT8},   {25, 2, BaseType::UINT16}, {26, 2, BaseType::UINT16},
	                   {254, 2, BaseType::UINT16}, {0, 1, BaseType::ENUM},    {1, 1, BaseType::ENUM},
	                   {5, 1, BaseType::ENUM},     {6, 1, BaseType::ENUM},    {28, 1, BaseType::ENUM}});
	buffer.DataHeader(LocalMessage::SESSION);
	buffer.U32(ToFitTime(endUnix));
	buffer.U32(ToFitTime(startUnix));
	WriteSummaryFields(buffer, ride);
	buffer.U16(0);
	buffer.U16(lapIndex);
	buffer.U16(0);
	buffer.U8(EVENT_SESSION);
	buffer.U8(EVENT_TYPE_STOP);
	buffer.U8(SPORT_CYCLING);
	buffer.U8(SUB_SPORT_INDOOR_CYCLING);
	buffer.U8(SESSION_TRIGGER_ACTIVITY_END);

	// Activity
	buffer.Definition(LocalMessage::ACTIVITY, GlobalMessage::ACTIVITY,
	                  {{253, 4, BaseType::UINT32},
	                   {0, 4, BaseType::UINT32},
	                   {1, 2, BaseType::UINT16},
	                   {2, 1, BaseType::ENUM},
	                   {3, 1, BaseType::ENUM},
	                   {4, 1, BaseType::ENUM},
	                   {5, 4, BaseType::UINT32}});
	buffer.DataHeader(LocalMessage::ACTIVITY);
	buffer.U32(ToFitTime(endUnix));
	buffer.U32(ScaledU32(ride.timerSeconds, 1000.0));
	buffer.U16(1);
	buffer.U8(ACTIVITY_TYPE_MANUAL);
	buffer.U8(EVENT_ACTIVITY);
	buffer.U8(EVENT_TYPE_STOP);
	buffer.U32(ToFitTime(endUnix + options.utcOffsetSeconds));

	// Header (14 bytes, protocol 1.0) + data + file CRC.
	const std::vector<uint8_t>& data = buffer.Bytes();
	std::vector<uint8_t> file;
	file.reserve(data.size() + 16);
	const uint32_t dataSize = (uint32_t)data.size();
	file.push_back(14);
	file.push_back(0x10);                    // protocol version 1.0
	file.push_back((uint8_t)(2132 & 0xFF));  // profile version 21.32
	file.push_back((uint8_t)(2132 >> 8));
	for (int i = 0; i < 4; ++i)
	{
		file.push_back((uint8_t)((dataSize >> (8 * i)) & 0xFF));
	}
	file.push_back('.');
	file.push_back('F');
	file.push_back('I');
	file.push_back('T');
	const uint16_t headerCrc = FitCrc(file.data(), 12);
	file.push_back((uint8_t)(headerCrc & 0xFF));
	file.push_back((uint8_t)(headerCrc >> 8));
	file.insert(file.end(), data.begin(), data.end());
	const uint16_t fileCrc = FitCrc(file.data(), file.size());
	file.push_back((uint8_t)(fileCrc & 0xFF));
	file.push_back((uint8_t)(fileCrc >> 8));
	return file;
}

// --- files -------------------------------------------------------------------------------------------------------

static std::filesystem::path PathFromUtf8(const std::string& text)
{
	return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

static bool WriteBytesAtomically(const std::string& path, const char* data, size_t size, std::string& error)
{
	const std::filesystem::path target = PathFromUtf8(path);
	std::filesystem::path temporary    = target;
	temporary += ".tmp";
	std::error_code filesystemError;
	std::filesystem::create_directories(target.parent_path(), filesystemError);
	{
		std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
		if (!file)
		{
			error = "can't create " + temporary.string();
			return false;
		}
		file.write(data, (std::streamsize)size);
		file.flush();
		if (!file)
		{
			error = "write failed for " + temporary.string();
			return false;
		}
	}
	std::filesystem::rename(temporary, target, filesystemError);
	if (filesystemError)
	{
		error = "can't replace " + target.string() + ": " + filesystemError.message();
		return false;
	}
	return true;
}

bool WriteFileAtomically(const std::string& path, const std::vector<uint8_t>& bytes, std::string& error)
{
	return WriteBytesAtomically(path, (const char*)bytes.data(), bytes.size(), error);
}

bool WriteFileAtomically(const std::string& path, const std::string& text, std::string& error)
{
	return WriteBytesAtomically(path, text.data(), text.size(), error);
}
}  // namespace Struktur::Ride
