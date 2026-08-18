#pragma once

#include <string>
#include <vector>

#include "Engine/Util/Color.h"

namespace Struktur
{
namespace Text
{
// Bitmask of simultaneously-active animated effects (see ParseMarkup's [wave]/[shake]/[pulse] tags) - a
// bitmask rather than a single enum value so effects layer (e.g. [shake][pulse]text[/pulse][/shake] sets
// both bits on the same run) instead of one replacing another. Applied per-glyph, per-frame, on the GPU (see
// vs_richTextAnim.sc) - see the rich-text design doc's Phase 8 addendum for why this is shader-driven rather
// than CPU-side like the rest of UIRichLabel.
using AnimationMask = uint8_t;
namespace AnimationFlags
{
constexpr AnimationMask Wave     = 1 << 0;
constexpr AnimationMask Shake    = 1 << 1;
constexpr AnimationMask Pulse    = 1 << 2;
constexpr AnimationMask Rainbow  = 1 << 3;
constexpr AnimationMask Tornado  = 1 << 4;
constexpr AnimationMask Fade     = 1 << 5;
}  // namespace AnimationFlags

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

	// Mirrors Renderer::AnimationUniformValues field-for-field (see UIRichLabel, which copies these straight
	// across when building an animated draw group) rather than a generic param struct, so each effect type
	// keeps its own independent values even when several are layered on one run - a run with both wave and
	// shake active needs both sets of numbers simultaneously, not one shared "amplitude/frequency" pair. An
	// inactive effect's fields are simply never read (see effectMask) - defaults here match Godot's own
	// example values for each tag, so a bare [wave] with no explicit params still looks reasonable.
	AnimationMask effectMask  = 0;
	float waveAmplitude       = 20.0f;
	float waveFrequency       = 5.0f;
	float shakeRate           = 20.0f;
	float shakeLevel          = 3.0f;
	float pulseFrequency      = 1.0f;
	float rainbowFrequency    = 1.0f;
	float rainbowSaturation   = 0.8f;
	float rainbowValue        = 0.8f;
	float tornadoRadius       = 10.0f;
	float tornadoFrequency    = 1.0f;
	// fadeStart/fadeLength are character-index offsets (not seconds - fade is the one effect with no time
	// term at all, see vs_richTextAnim.sc's own comment), measured against the same per-*label* (not
	// per-run) animCharIndex every other effect uses (see AppendAnimatedGlyphs) - a simplification versus
	// Godot's own per-BBCode-block-relative [fade], made because the vertex format only carries one shared
	// charIndex field. Characters before fadeStart are fully opaque, characters from fadeStart to
	// fadeStart+fadeLength ramp linearly to transparent, everything after stays fully transparent.
	float fadeStart           = 0.0f;
	float fadeLength          = 8.0f;
};

// One line of markup, already split on '\n' - mirrors UILabel::GetTextLines' NONE-wrap splitting, just
// operating on parsed runs instead of a raw string. Word-wrapping happens *after* parsing, over each run's
// already-tag-free text (see TextLayout::WrapText) - RichLine itself never re-wraps.
struct RichLine
{
	std::vector<RichRun> runs;
};

// Parses BBCode-style markup ([b]...[/b], [i]...[/i], [color=#rrggbb]...[/color], [icon=name],
// [wave amp=20 freq=5]...[/wave], [shake rate=20 level=3]...[/shake], [pulse freq=1]...[/pulse],
// [rainbow freq=1 sat=0.8 val=0.8]...[/rainbow], [tornado radius=10 freq=1]...[/tornado],
// [fade start=0 length=8]...[/fade]) into a list of lines of styled runs. baseColor seeds the color every run
// starts with before any [color] tag is opened, and is what an unbalanced closing [/color] (more closes than
// opens) falls back to. Animation tag params are all optional (a bare tag uses RichRun's own default values);
// unrecognised param names inside an animation tag are ignored rather than causing the whole tag to fall back
// to literal, matching the general "be forgiving" stance below.
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
