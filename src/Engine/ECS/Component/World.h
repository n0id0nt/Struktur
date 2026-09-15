#pragma once

#include <unordered_map>

#include "entt/entt.hpp"

#include "Engine/FileLoading/LevelParser.h"

namespace Struktur
{
namespace Component
{
struct World
{
	FileLoading::LevelParser::World worldMap;
	// Levels currently loaded as entities, keyed by their index in worldMap.levels.
	std::unordered_map<int, entt::entity> loadedLevels;
};
}  // namespace Component
}  // namespace Struktur
