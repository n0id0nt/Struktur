#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Ride/RideLog.h"

namespace Struktur::Ride
{
struct FitOptions
{
	int32_t utcOffsetSeconds = 0;  // for the activity's local_timestamp
	uint16_t productId       = 1;  // manufacturer is "development" (255)
	uint32_t serialNumber    = 1;
};

// Minimal FIT activity encoder, written from the public FIT protocol description (no Garmin SDK code):
//   file_id, event (timer start / stop_all around pauses), record (1 Hz: timestamp, power, heart rate, cadence,
//   speed, distance, altitude), lap (one per workout step / manual lap), session, activity.
// Little-endian, protocol 1.0, CRC-16 over header and data.
std::vector<uint8_t> EncodeFitActivity(const RideLog& log, const FitOptions& options = FitOptions());

// The FIT CRC-16 (exposed for tests and the validator).
uint16_t FitCrc(const uint8_t* data, size_t size, uint16_t crc = 0);

// FIT "date_time": seconds since 1989-12-31 00:00 UTC.
uint32_t ToFitTime(int64_t unixTime);

// Writes `bytes` to `path` through a temporary file and a rename, so a crash mid-write never leaves a torn file
// (the previous autosave stays intact). Returns false and fills error on failure.
bool WriteFileAtomically(const std::string& path, const std::vector<uint8_t>& bytes, std::string& error);
bool WriteFileAtomically(const std::string& path, const std::string& text, std::string& error);
}  // namespace Struktur::Ride
