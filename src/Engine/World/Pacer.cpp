#include "Pacer.h"

#include <cmath>

namespace Struktur::World
{
void MovePacer(Component::Pacer& pacer, Component::RoadPosition& position, double riderDistance, double deltaSeconds)
{
	if (!pacer.placed)
	{
		position.distance = riderDistance + pacer.startAhead;
		pacer.placed      = true;
	}
	pacer.time += deltaSeconds;
	pacer.speedMetersPerSecond = 8.3 + 1.2 * std::sin(pacer.time * 0.05);
	position.distance += pacer.speedMetersPerSecond * deltaSeconds;
	if (position.distance < riderDistance - 80.0 || position.distance > riderDistance + 600.0)
	{
		position.distance = riderDistance + pacer.startAhead;
	}
}
}  // namespace Struktur::World
