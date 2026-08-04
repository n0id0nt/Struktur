#pragma once

namespace Struktur
{
namespace GameResource
{
// Coarse draw-order buckets. Ascending value = drawn first (further back).
// Entities sits between the two background buckets and the overlay/foreground buckets so it can
// y-sort against whichever tile layers are meant to sit immediately behind/in front of it.
enum class RenderLayer
{
	BackgroundFar,
	BackgroundMid,
	Entities,
	BackgroundOverlay,
	Foreground,
	UI,

	COUNT
};
}  // namespace GameResource
}  // namespace Struktur
