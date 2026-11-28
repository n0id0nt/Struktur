#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Struktur::Fitness
{
// Raw characteristic payload. Every multi-byte GATT field in the specs we implement is little-endian.
using Bytes = std::vector<uint8_t>;

// Bounds-checked little-endian reader over one characteristic value. Every Read* returns false (and leaves `out`
// untouched) once the payload runs short, so parsers can bail out of a truncated packet without UB.
class ByteReader
{
public:
	explicit ByteReader(std::span<const uint8_t> data)
	    : m_data(data),
	      m_offset(0)
	{
	}

	bool ReadU8(uint8_t& out)
	{
		if (Remaining() < 1)
		{
			return false;
		}
		out = m_data[m_offset];
		m_offset += 1;
		return true;
	}

	bool ReadU16(uint16_t& out)
	{
		if (Remaining() < 2)
		{
			return false;
		}
		out = static_cast<uint16_t>(m_data[m_offset] | (m_data[m_offset + 1] << 8));
		m_offset += 2;
		return true;
	}

	bool ReadS16(int16_t& out)
	{
		uint16_t raw = 0;
		if (!ReadU16(raw))
		{
			return false;
		}
		out = static_cast<int16_t>(raw);
		return true;
	}

	bool ReadU24(uint32_t& out)
	{
		if (Remaining() < 3)
		{
			return false;
		}
		out = static_cast<uint32_t>(m_data[m_offset]) | (static_cast<uint32_t>(m_data[m_offset + 1]) << 8) |
		      (static_cast<uint32_t>(m_data[m_offset + 2]) << 16);
		m_offset += 3;
		return true;
	}

	bool ReadU32(uint32_t& out)
	{
		if (Remaining() < 4)
		{
			return false;
		}
		out = static_cast<uint32_t>(m_data[m_offset]) | (static_cast<uint32_t>(m_data[m_offset + 1]) << 8) |
		      (static_cast<uint32_t>(m_data[m_offset + 2]) << 16) | (static_cast<uint32_t>(m_data[m_offset + 3]) << 24);
		m_offset += 4;
		return true;
	}

	size_t Remaining() const
	{
		return m_data.size() - m_offset;
	}

	std::span<const uint8_t> Rest() const
	{
		return m_data.subspan(m_offset);
	}

private:
	std::span<const uint8_t> m_data;
	size_t m_offset;
};

// Little-endian builder for control point commands.
class ByteWriter
{
public:
	ByteWriter& U8(uint8_t value)
	{
		m_bytes.push_back(value);
		return *this;
	}

	ByteWriter& U16(uint16_t value)
	{
		m_bytes.push_back(static_cast<uint8_t>(value & 0xFF));
		m_bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
		return *this;
	}

	ByteWriter& S16(int16_t value)
	{
		return U16(static_cast<uint16_t>(value));
	}

	Bytes Take()
	{
		return std::move(m_bytes);
	}

private:
	Bytes m_bytes;
};
}  // namespace Struktur::Fitness
