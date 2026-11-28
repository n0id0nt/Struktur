#pragma once

#include "World/WorldSettings.h"

namespace Struktur
{
class GameContext;
}

// The whole world as the game sees it: the World itself, its ECS entities and the renderer's GPU resources. Scripts
// reach these through the "world" module.
namespace Struktur::World
{
// Replaces any loaded world. `demoPacer` adds a pacer rider. False (and nothing loaded) when the settings disable the
// world or there is no biome data.
bool LoadWorld(GameContext& context, const WorldSettings& settings, bool demoPacer);
void UnloadWorld(GameContext& context);
// A finish banner `aheadMeters` in front of the player (no-op while nothing is loaded).
void PlaceFinishBanner(GameContext& context, double aheadMeters);
}  // namespace Struktur::World
