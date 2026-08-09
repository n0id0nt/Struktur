#pragma once

#if !defined(PLATFORM_WEB)

#include <cstdint>

#include "Engine/Util/MathUtil.h"

#include <bgfx/bgfx.h>

namespace Struktur
{
namespace Renderer
{
// A cached, static mesh covering a fixed-size block of a tilemap's tiles - built once (see TileChunkBuilder)
// instead of every tile being rebuilt into a transient buffer each frame like a moving sprite.
struct TileChunk
{
	bgfx::VertexBufferHandle vb = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle ib  = BGFX_INVALID_HANDLE;
	uint32_t indexCount         = 0;
	// World-space AABB (tile positions already have the tilemap entity's world offset baked in), used to
	// cull whole chunks against the camera instead of testing every tile individually.
	Util::Math::Rect worldBounds{};
};
}  // namespace Renderer
}  // namespace Struktur

#endif
