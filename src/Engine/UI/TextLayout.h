#pragma once

#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Struktur
{
namespace UI
{
enum class TextAlignment
{
	LEFT,
	CENTER,
	RIGHT,
	JUSTIFY
};

enum class TextWrapping
{
	NONE,
	WORD_WRAP,
	CHARACTER_WRAP
};

// Font/string-agnostic text layout helpers, extracted out of UILabel so a future multi-font/multi-run caller
// (a rich-text label mixing bold/italic/color spans on one line) can reuse the exact same wrap/alignment math
// a single-font UILabel uses, instead of re-deriving it. Every function here is a pure function of
// already-measured widths/positions - none of them know what a Text::Font or a FontResource is.
namespace TextLayout
{
// Word/character-wraps text to maxWidth, calling measureWidth(candidateLine) to measure each candidate line
// rather than assuming one font - same greedy accumulate-then-break algorithm UILabel::WrapText always used,
// just with the measurement step pulled out to a callback so a caller juggling multiple fonts (rich text) can
// supply per-run-aware measurement instead of a single MeasureTextEx(font, ...) call.
std::vector<std::string> WrapText(const std::string& text, float maxWidth, TextWrapping mode,
                                  const std::function<float(const std::string&)>& measureWidth);

// Splits a line on spaces/tabs into words - the same tokenising RenderJustifiedLine and WrapText's word-wrap
// mode both need before they can measure/position word-by-word.
std::vector<std::string> SplitWords(const std::string& line);

// Start-X for one line of lineWidth within a box at [boxX, boxX+boxWidth), given alignment and the flat
// padding used on every other side. JUSTIFY resolves the same as LEFT here - a justified line needs its own
// per-word spacing (see ComputeJustifySpaceWidth), not a single start-X.
float ComputeAlignedStartX(TextAlignment alignment, float boxX, float boxWidth, float lineWidth, float padding);

// Extra space to insert between each pair of words so wordCount words spanning totalWordWidth exactly fill
// targetWidth. Caller must ensure wordCount > 1 - a single word can't be justified (divides by wordCount-1).
float ComputeJustifySpaceWidth(float targetWidth, float totalWordWidth, size_t wordCount);

// Reduces per-line (width, height) measurements to one bounding size: width is the max line width, height is
// the sum of line heights - each axis gets its own flat padding added once at the end, not per line. Line
// heights are passed per-line (not a single shared value) so a caller with per-line-varying height (e.g. a
// line mixing a tall icon and small text) can feed its own per-line max straight in, while a single-font
// caller just repeats the same height once per line.
glm::vec2 SumLineBounds(const std::vector<float>& lineWidths, const std::vector<float>& lineHeights,
                        float paddingX, float paddingY);

// One indivisible unit of wrappable content (a whole word plus its trailing space/tab character, for
// WORD_WRAP; a single character, for CHARACTER_WRAP) with its pre-measured width and an opaque caller-defined
// styleId, round-tripped from input to output verbatim by WrapTokens. Exists because WrapText's single
// measureWidth(candidate-line-as-one-string) callback can't express a line whose different words need
// different fonts to measure (e.g. a rich-text run of bold words next to regular ones) - a token carries its
// own pre-measured width instead, so WrapTokens never needs to know how any of it was measured.
struct WrapToken
{
	std::string text;
	float width = 0.0f;
	int styleId = 0;
};

// Splits text into WrapTokens per mode - WORD_WRAP yields one token per word (each including its own single
// trailing space/tab, matching WrapText's own word+separator chunking), CHARACTER_WRAP yields one token per
// (byte) character, matching WrapText's own non-UTF-8-aware character splitting. Every token produced is
// stamped with styleId - a multi-run caller tokenizes each of its runs separately (typically styleId = that
// run's index) and concatenates the results into one token stream spanning multiple styles before calling
// WrapTokens.
std::vector<WrapToken> Tokenize(const std::string& text, TextWrapping mode, int styleId,
                                const std::function<float(const std::string&)>& measureWidth);

// Same greedy accumulate-then-break algorithm WrapText uses, generalized to pre-tokenized/pre-measured input:
// width is summed token-by-token as each is added to the current line, rather than the whole candidate line
// being re-measured as one string - exactly equivalent to WrapText's own per-line remeasurement as long as
// there's no cross-token kerning, and Text::MeasureTextEx never does any (a pure per-glyph advance sum). Each
// returned line is itself a list of tokens in original order; a token is never split across a line break.
std::vector<std::vector<WrapToken>> WrapTokens(const std::vector<WrapToken>& tokens, float maxWidth);
}  // namespace TextLayout
}  // namespace UI
}  // namespace Struktur
