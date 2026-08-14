#include "CollisionLayers.h"

#include <format>

#include "Debug/Assertions.h"

uint16_t Struktur::Physics::CollisionLayers::RegisterLayer(const std::string& name)
{
	auto it = m_layers.find(name);
	if (it != m_layers.end())
	{
		return it->second;
	}

	if (m_nextBit == 0)
	{
		BREAK_MSG(std::format("CollisionLayers: no free bits left to register layer '{}' (16 layers already in use)",
		                     name)
		              .c_str());
		return 0;
	}

	uint16_t bit      = m_nextBit;
	m_layers[name]    = bit;
	m_layerNames[bit] = name;
	// Doubles toward the next single bit each call - wraps to 0 once it shifts past bit 15, signalling "no room
	// left" on the next RegisterLayer call above.
	m_nextBit = (uint16_t)(m_nextBit << 1);

	return bit;
}

uint16_t Struktur::Physics::CollisionLayers::GetLayer(const std::string& name) const
{
	auto it = m_layers.find(name);
	return it != m_layers.end() ? it->second : 0;
}

const std::string& Struktur::Physics::CollisionLayers::GetLayerName(uint16_t bit) const
{
	static const std::string kEmpty;
	auto it = m_layerNames.find(bit);
	return it != m_layerNames.end() ? it->second : kEmpty;
}

bool Struktur::Physics::CollisionLayers::HasLayer(const std::string& name) const
{
	return m_layers.find(name) != m_layers.end();
}

uint16_t Struktur::Physics::CollisionLayers::RegisterGroup(const std::string& groupName,
                                                           const std::vector<std::string>& layerNames)
{
	uint16_t mask = 0;
	for (const std::string& layerName : layerNames)
	{
		mask |= RegisterLayer(layerName);
	}

	m_groups[groupName] = mask;
	return mask;
}

uint16_t Struktur::Physics::CollisionLayers::GetGroup(const std::string& groupName) const
{
	auto it = m_groups.find(groupName);
	return it != m_groups.end() ? it->second : 0;
}

bool Struktur::Physics::CollisionLayers::HasGroup(const std::string& groupName) const
{
	return m_groups.find(groupName) != m_groups.end();
}
