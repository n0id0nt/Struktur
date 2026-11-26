#include <cstdio>

#include "Engine/Fitness/Ble/IBleCentral.h"

namespace Struktur::Fitness
{
std::string FormatAddress(BleAddress address)
{
	char buffer[18];
	std::snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
	              static_cast<unsigned>((address >> 40) & 0xFF), static_cast<unsigned>((address >> 32) & 0xFF),
	              static_cast<unsigned>((address >> 24) & 0xFF), static_cast<unsigned>((address >> 16) & 0xFF),
	              static_cast<unsigned>((address >> 8) & 0xFF), static_cast<unsigned>(address & 0xFF));
	return std::string(buffer);
}

bool ParseAddress(const std::string& text, BleAddress& out)
{
	if (text.size() != 17)
	{
		return false;
	}
	BleAddress address = 0;
	for (size_t i = 0; i < text.size(); ++i)
	{
		const char character = text[i];
		if (i % 3 == 2)
		{
			if (character != ':' && character != '-')
			{
				return false;
			}
			continue;
		}
		int nibble = -1;
		if (character >= '0' && character <= '9')
		{
			nibble = character - '0';
		}
		else if (character >= 'a' && character <= 'f')
		{
			nibble = character - 'a' + 10;
		}
		else if (character >= 'A' && character <= 'F')
		{
			nibble = character - 'A' + 10;
		}
		if (nibble < 0)
		{
			return false;
		}
		address = (address << 4) | static_cast<BleAddress>(nibble);
	}
	out = address;
	return true;
}

const char* ToString(BleStatus status)
{
	switch (status)
	{
		case BleStatus::SUCCESS:
			return "Success";
		case BleStatus::NOT_AVAILABLE:
			return "Bluetooth not available";
		case BleStatus::NOT_FOUND:
			return "Not found";
		case BleStatus::UNREACHABLE:
			return "Unreachable";
		case BleStatus::PROTOCOL_ERROR:
			return "Protocol error";
		case BleStatus::ACCESS_DENIED:
			return "Access denied";
		case BleStatus::NOT_CONNECTED:
			return "Not connected";
		case BleStatus::UNKNOWN_ERROR:
			return "Unknown error";
	}
	return "Unknown status";
}
}  // namespace Struktur::Fitness
