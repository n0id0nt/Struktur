#pragma once

namespace Struktur
{
namespace Renderer
{
// Bit 0 = horizontal flip, bit 1 = vertical flip - matches LDtk's own tile flip flag encoding directly (see
// FileLoading::LevelParser's own FlipBit, cast into this one when tiles are loaded from a level file). Shared
// between Component::TileMap and Component::Sprite - both mirror a source-atlas rect the same way (see
// TileChunkBuilder/SpriteRenderSystem's own "flip via source-rect" comment) so there is only one flip concept
// to keep in sync between them.
enum class FlipBit
{
	NONE       = 0,
	HORIZONTAL = 1,
	VERTICAL   = 2,
	BOTH       = 3,

	COUNT
};
}  // namespace Renderer
}  // namespace Struktur
