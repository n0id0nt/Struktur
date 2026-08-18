#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>

namespace Struktur
{
namespace Renderer
{
// Vertex format for animated rich-text glyph quads (see UIRichLabel's design doc, Phase 8) - everything
// QuadVertex has, plus one extra packed attribute carrying per-glyph animation data. Deliberately a separate
// format/vertex buffer from QuadVertex/UIBatch rather than an extension of the shared format: only animated
// glyphs ever need this data, so keeping it out of QuadVertex means the other 99% of UI quads (backgrounds,
// borders, icons, plain text) pay zero extra bandwidth for a feature they never use.
struct AnimQuadVertex
{
	float x, y;
	float u, v;
	uint32_t abgr;
	// Packed as 4 unnormalized uint8 components (delivered to the vertex shader as a plain vec4 with raw 0-255
	// values, NOT bgfx's asInt integer vertex-fetch path) - asInt attributes aren't supported on every profile
	// this project targets (notably GLES2/the "100_es" web profile, see CMakeLists.txt's shader-compile
	// comment), so this deliberately avoids relying on it. byte0 = effectMask (bitmask, see
	// Text::RichRun::effectMask), byte1/byte2 = charIndex low/high (charIndex = byte2*256+byte1, up to 65535 -
	// far more than any realistic UI string), byte3 = reserved/unused. The vertex shader reconstructs each
	// field with plain arithmetic (no bitwise ops, no division/modulo needed since bgfx already splits the
	// bytes for us) - see vs_richTextAnim.sc.
	uint8_t animEffectMask;
	uint8_t animCharIndexLow;
	uint8_t animCharIndexHigh;
	uint8_t animReserved;
};

inline bgfx::VertexLayout BuildAnimQuadVertexLayout()
{
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .add(bgfx::Attrib::TexCoord1, 4, bgfx::AttribType::Uint8, false)
	    .end();
	return layout;
}

// Packs a glyph's animation data into the 4 bytes AnimQuadVertex::animEffectMask.. carries - charIndex is
// clamped to 65535 (see the field comment above) rather than wrapping, so a pathologically long animated run
// degrades to "later glyphs share the last valid phase" instead of visibly wrapping back to index 0.
inline void PackAnimData(uint8_t effectMask, int charIndex, uint8_t& outMask, uint8_t& outLow, uint8_t& outHigh)
{
	int clampedIndex = charIndex < 0 ? 0 : (charIndex > 65535 ? 65535 : charIndex);
	outMask           = effectMask;
	outLow            = (uint8_t)(clampedIndex & 0xFF);
	outHigh           = (uint8_t)((clampedIndex >> 8) & 0xFF);
}
}  // namespace Renderer
}  // namespace Struktur
