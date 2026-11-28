#include "BleUuid.h"

#include <cstdio>

namespace Struktur::Fitness
{
static int HexDigitValue(char digit)
{
	if (digit >= '0' && digit <= '9')
	{
		return digit - '0';
	}
	if (digit >= 'a' && digit <= 'f')
	{
		return digit - 'a' + 10;
	}
	if (digit >= 'A' && digit <= 'F')
	{
		return digit - 'A' + 10;
	}
	return -1;
}

std::optional<uint16_t> BleUuid::GetShort() const
{
	const BleUuid base = FromShort(0);
	for (size_t i = 0; i < bytes.size(); ++i)
	{
		if (i == 2 || i == 3)
		{
			continue;
		}
		if (bytes[i] != base.bytes[i])
		{
			return std::nullopt;
		}
	}
	return static_cast<uint16_t>((bytes[2] << 8) | bytes[3]);
}

std::string BleUuid::ToString() const
{
	char buffer[37];
	std::snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	              bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9],
	              bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
	return std::string(buffer);
}

std::optional<BleUuid> BleUuid::Parse(std::string_view text)
{
	if (text.size() == 4)
	{
		uint16_t value = 0;
		for (char digit : text)
		{
			const int nibble = HexDigitValue(digit);
			if (nibble < 0)
			{
				return std::nullopt;
			}
			value = static_cast<uint16_t>((value << 4) | nibble);
		}
		return FromShort(value);
	}

	if (text.size() != 36)
	{
		return std::nullopt;
	}

	BleUuid uuid;
	size_t byteIndex = 0;
	for (size_t i = 0; i < text.size();)
	{
		if (i == 8 || i == 13 || i == 18 || i == 23)
		{
			if (text[i] != '-')
			{
				return std::nullopt;
			}
			++i;
			continue;
		}
		const int high = HexDigitValue(text[i]);
		const int low  = HexDigitValue(text[i + 1]);
		if (high < 0 || low < 0 || byteIndex >= uuid.bytes.size())
		{
			return std::nullopt;
		}
		uuid.bytes[byteIndex++] = static_cast<uint8_t>((high << 4) | low);
		i += 2;
	}
	return byteIndex == uuid.bytes.size() ? std::optional<BleUuid>(uuid) : std::nullopt;
}
}  // namespace Struktur::Fitness
