#pragma once

#include "Components/WorldComponents.h"

namespace Struktur::World
{
// Moves a pacer for one frame: a steady 30 km/h with a slow surge, so the player can catch it and drop it. Far behind
// or far ahead of the player, it rejoins a little in front so there is always someone to look at.
void MovePacer(Component::Pacer& pacer, Component::RoadPosition& position, double riderDistance, double deltaSeconds);
}  // namespace Struktur::World
