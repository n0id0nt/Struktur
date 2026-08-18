#pragma once

#include <string>
#include <vector>

#include "Engine/Util/Color.h"

namespace Struktur
{
namespace Text
{
// One contiguously-styled span of text (or a single inline icon) within a RichLine. `text` is drawn glyph by
// glyph the same way UILabel already draws a plain string (see UIRenderer::DrawText) - `iconName` is set
// instead for a `[icon=name]` tag, in which case `text` is always empty and the run represents one inline
// icon quad rather than font glyphs (see the icon-atlas plan in the rich-text design doc).
struct RichRun
{
	std::string text;
	bool bold   = false;
	bool italic = false;
	Util::Color color{};
	std::string iconName;  // non-empty => this run is an icon, `text` is ignored
};

// One line of markup, already split on '\n' - mirrors UILabel::GetTextLines' NONE-wrap splitting, just
// operating on parsed runs instead of a raw string. Word-wrapping happens *after* parsing, over each run's
// already-tag-free text (see TextLayout::WrapText) - RichLine itself never re-wraps.
struct RichLine
{
	std::vector<RichRun> runs;
};

// Parses BBCode-style markup ([b]...[/b], [i]...[/i], [color=#rrggbb]...[/color], [icon=name]) into a list of
// lines of styled runs. baseColor seeds the color every run starts with before any [color] tag is opened, and
// is what an unbalanced closing [/color] (more closes than opens) falls back to.
//
// Deliberately lenient rather than strict, matching the reveal-effect use case (a caller progressively
// re-parsing a growing prefix of the same markup string every frame - see UIRichLabel's planned
// setVisibleGlyphCount) as well as most BBCode dialects: an unknown tag name, a malformed `[` with no
// matching `]`, a mismatched/stray closing tag, or a [color=] with an unparseable argument are all treated as
// literal text (the tag's own source characters, verbatim) rather than raising an error - so a truncated or
// mistyped tag degrades to visible garbage a writer can spot, never a crash.
std::vector<RichLine> ParseMarkup(const std::string& markup, Util::Color baseColor);

// Parses a "#rrggbb" or "#rrggbbaa" hex string (leading '#' required) into outColor. Returns false and leaves
// outColor untouched if hex isn't exactly 6 or 8 valid hex digits after the '#'.
bool TryParseHexColor(const std::string& hex, Util::Color& outColor);
}  // namespace Text
}  // namespace Struktur
