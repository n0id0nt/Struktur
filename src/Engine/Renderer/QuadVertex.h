#pragma once

#include <cstdint>

#include "Engine/Util/Color.h"

#include <bgfx/bgfx.h>

namespace Struktur
{
namespace Renderer
{
// Shared vertex format for every textured quad drawn into the world view - individual sprites (WorldRenderer's
// transient per-frame batches) and cached tile chunks (TileChunkBuilder's static meshes) alike.
struct QuadVertex
{
	float x, y;
	float u, v;
	uint32_t abgr;
};

inline bgfx::VertexLayout BuildQuadVertexLayout()
{
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	return layout;
}

inline uint32_t PackColor(const Util::Color& c)
{
	return ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.r;
}
}  // namespace Renderer
}  // namespace Struktur
