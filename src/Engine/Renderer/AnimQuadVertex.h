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
	// Packed as 4 NORMALIZED uint8 components (a real UNORM vertex format, the same one a_color0 already uses
	// successfully) - delivered to the vertex shader as a vec4 in 0.0-1.0 range, recovered back to the original
	// 0-255 byte values via *255.0 in vs_richTextAnim.sc. Deliberately NOT unnormalized (normalized=false,
	// asInt=false) despite that reading as the more "obvious" choice for raw integer bytes: Direct3D has no
	// vertex format for "unnormalized uint8, numerically converted to float" - only UNORM (real hardware
	// normalize-and-convert) or UINT (raw integer, requires an actual uint-typed shader register to read
	// correctly). With normalized=false, bgfx binds a UINT-format buffer to this float-typed shader input, and
	// the hardware satisfies that via bit-reinterpretation, not conversion - e.g. the byte value 6 arrives as
	// ~8.4e-45 (6 reinterpreted as a subnormal float), not 6.0. Confirmed via RenderDoc: the mesh viewer's VS
	// Input table correctly showed the intended packed bytes, but the vertex shader's actual watch-window value
	// for a_texcoord1 showed exactly this bit-reinterpretation pattern - every mod()/floor() bit test against
	// such a tiny near-zero value evaluates false unconditionally, regardless of what mask was really packed.
	// NOT bgfx's asInt integer vertex-fetch path either (a true UINT-format buffer bound to a uint-typed shader
	// register, which WOULD numerically round-trip correctly) - asInt attributes aren't supported on every
	// profile this project targets (notably GLES2/the "100_es" web profile, see CMakeLists.txt's shader-compile
	// comment). byte0 = effectMask (bitmask, see Text::RichRun::effectMask), byte1/byte2 = charIndex low/high
	// (charIndex = byte2*256+byte1, up to 65535 - far more than any realistic UI string), byte3 = reserved/
	// unused.
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
	    .add(bgfx::Attrib::TexCoord1, 4, bgfx::AttribType::Uint8, true)
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
