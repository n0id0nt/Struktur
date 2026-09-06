#include "Engine/Renderer/FlipBit.h"
#include "Engine/Scripting/WrenBindingRegistry.h"

// ============================================================================
// RENDERER BINDINGS - enums/types shared by the rendering side (sprites, tilemaps, world rendering)
// ============================================================================

WREN_BINDING_MODULE(Renderer)
{
	WREN_ENUM(registry, "renderer", FlipBit, "Sprite/tile source-rect mirroring - shared with TileMap",
	          WREN_ENUM_PAIR("NONE", Struktur::Renderer::FlipBit::NONE),
	          WREN_ENUM_PAIR("HORIZONTAL", Struktur::Renderer::FlipBit::HORIZONTAL),
	          WREN_ENUM_PAIR("VERTICAL", Struktur::Renderer::FlipBit::VERTICAL),
	          WREN_ENUM_PAIR("BOTH", Struktur::Renderer::FlipBit::BOTH), );
}
