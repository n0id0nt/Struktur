#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Engine/Util/MathUtil.h"

namespace Struktur
{
namespace Text
{
// Replaces raylib's ::GlyphInfo - one baked glyph's metrics plus its rect within the owning Font's atlas texture.
struct Glyph
{
	int value    = 0;  // codepoint
	int offsetX  = 0;
	int offsetY  = 0;
	int advanceX = 0;
	Util::Math::Rect rec{};  // atlas-space rect (replaces raylib's parallel Font::recs[] array)
};

// Replaces raylib's ::Texture2D as far as Font needs it - just enough to resolve a bgfx texture handle by index
// (see TextureResource::GetHandle()'s "id doubles as the bgfx handle's idx" convention) and know its dimensions
// for UV math.
struct TextureRef
{
	unsigned int id = 0;
	int width       = 0;
	int height      = 0;
};

// Replaces raylib's ::Font. FontResource populates this by hand on every platform already (stb_truetype baking,
// not a raylib font loader), so this is a direct rename, not a new loading path.
struct Font
{
	int baseSize = 0;
	std::vector<Glyph> glyphs;
	TextureRef texture;
};

// Replaces raylib's ::GetCodepointNext - decodes one UTF-8 codepoint starting at text, writes the number of
// bytes consumed to *codepointSize. Invalid/truncated sequences decode as U+FFFD and consume 1 byte, matching
// raylib's own fallback behaviour.
int GetCodepointNext(const char* text, int* codepointSize);

// Replaces raylib's ::GetGlyphIndex - linear scan for a matching codepoint; falls back to the '?' glyph (or
// index 0 if '?' isn't in the set) when not found, matching raylib's own fallback behaviour.
int GetGlyphIndex(const Font& font, int codepoint);

// Replaces raylib's ::MeasureTextEx - returns {width, height} for a single-line string at the given size/spacing.
glm::vec2 MeasureTextEx(const Font& font, const std::string& text, float fontSize, float spacing);
}  // namespace Text
}  // namespace Struktur
