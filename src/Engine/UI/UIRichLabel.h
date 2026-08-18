#pragma once

#include <string>
#include <vector>

#include "Engine/Renderer/AnimQuadVertex.h"
#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Text/RichText.h"
#include "Engine/UI/IconAtlas.h"
#include "Engine/UI/TextLayout.h"
#include "Engine/UI/UIElement.h"
#include "Engine/Util/Color.h"

namespace Struktur
{
class GameContext;

namespace UI
{
// One animated-glyph draw group accumulated during one Render() pass (see UIRichLabel::RenderLine) - every
// run sharing the same (texture, effect bitmask, resolved params) ends up in one group's vertex list,
// regardless of whether the runs were textually adjacent in the source markup, so they submit as a single
// UIRenderer AnimatedTextBatch/draw call rather than one per run. See the Phase 8 design doc for why animated
// glyphs live in their own vertex format/batch rather than the regular UIBatch static-text path.
struct UIRichLabelAnimGroup
{
	bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
	uint8_t effectMask          = 0;
	float waveAmplitude         = 0.0f;
	float waveFrequency         = 0.0f;
	float shakeRate             = 0.0f;
	float shakeLevel            = 0.0f;
	float pulseFrequency        = 0.0f;
	float rainbowFrequency      = 0.0f;
	float rainbowSaturation     = 0.0f;
	float rainbowValue          = 0.0f;
	float tornadoRadius         = 0.0f;
	float tornadoFrequency      = 0.0f;
	float fadeStart             = 0.0f;
	float fadeLength            = 0.0f;
	std::vector<Renderer::AnimQuadVertex> vertices;
};
//=============================================================================
// UIRichLabel - markup-driven text display element
//=============================================================================
// Supports inline [b]/[i]/[color=#rrggbb] formatting (see Text::ParseMarkup) by fanning a line out into one
// UIRenderer::DrawText call per styled run rather than one call for the whole line, each with its own
// font/color and advancing the pen by that run's own measured width (Text::MeasureTextEx) before the next -
// see the rich-text design doc for why this needs no UIRenderer/UIBatch/FontResource changes at all.
//
// Deliberately a UIElement sibling to UILabel, not a subclass of it - UILabel's actual rendering/wrap/measure
// logic is private and non-virtual and hard-wired to one font+string, so inheriting from it wouldn't reach
// anything reusable. The font-agnostic layout mechanics both classes need (wrap-by-width, alignment math,
// bounds summation) live in TextLayout.h instead and are reused by direct calls, not inheritance - this class
// doesn't need them yet (see below) but will when wrapping/alignment land.
//
// Current scope (see the design doc's phased build order - this is step 7 of 7): multi-run rendering with
// bold/italic/color, one hard line per literal '\n' in the source markup (Text::ParseMarkup already splits on
// those) further soft-wrapped to fit the label's width via TextLayout::Tokenize/WrapTokens - the same
// greedy accumulate-then-break algorithm UILabel::WrapText uses, generalized to a multi-font token stream
// (see TextLayout.h). Each wrapped line's height is the max over that line's runs' natural heights (all runs
// currently resolve to the same m_fontSize*1.5/IconSize() height today, since no run yet has its own font
// size - see ComputeLineHeight - but the per-line-max plumbing is in place for when per-run sizing lands).
//
// [icon=name] runs render as a square IconSize() x IconSize() quad sampling a named sub-rect of an
// IconAtlas (see SetIconAtlas) via UIRenderer::DrawTexturedRectRegion, tinted white so the icon's own source
// pixels pass through unmodified - unlike text runs, icons keep their own color rather than being tinted by
// the run's parsed [color=] (icons are typically already-colored art, e.g. a controller button glyph). An
// icon whose name isn't registered in the current atlas (or when no atlas is set at all) simply isn't drawn -
// no quad, no pen advance, no error - so a label written against markup that references icons still renders
// its text correctly before any atlas is wired up.
//
// A run needing italics that has no dedicated m_italicFont/m_boldItalicFont set gets a synthetic slant
// instead of silently rendering upright - a horizontal shear applied to its glyph quads (see ResolveFont's
// outIsRealItalic and UIRenderer::DrawText's shearAmount), not a real font transform. A dedicated italic
// FontResource is always preferred when set; the synthetic path only ever fires as a fallback. Bold has no
// equivalent synthetic fallback - a bold run with no m_boldFont/m_boldItalicFont just renders in the regular
// weight, matching Godot's own stance that synthesized bold specifically "rarely looks as good" as a real
// font variant (synthetic italic via shear holds up far better than synthetic bold via stroke-thickening).
//
// [wave]/[shake]/[pulse] runs (see Text::ParseMarkup, Phase 8) render through a completely separate path from
// everything above: their glyph quads use AnimQuadVertex (not QuadVertex) and go into their own
// UIRenderer::AnimatedTextBatch/es (see m_animBatches) instead of m_batchSlot, submitted with the
// vs/fs_richTextAnim shader pair, which does the actual per-character displacement/color work on the GPU
// every frame from a `charIndex` baked into each vertex plus a live time uniform - see the design doc's Phase
// 8 addendum for why this is shader-driven (zero per-frame CPU rebuild) rather than CPU-side like the rest of
// this class. GetRequiredQuadCount()/m_batchSlot only ever account for non-animated glyphs; animated runs are
// entirely invisible to that bookkeeping. Icon runs never animate regardless of any enclosing effect tag.
//
// Deliberately NOT yet implemented: text alignment (this always renders left-aligned), animated icons, and
// synthetic-italic shear on animated runs (an animated run needing italics with no dedicated font just
// renders upright - shear is only wired into the static UIRenderer::DrawText path today).
class UIRichLabel : public UIElement
{
private:
	std::string m_markupText;
	std::vector<Text::RichLine> m_parsedLines;  // Text::ParseMarkup(m_markupText, m_baseColor), kept in sync by
	                                            // Reparse() - never re-parsed inside Render()/GetRequiredQuadCount().

	Resource::ResourcePtr<Resource::FontResource> m_font;         // regular - always set (see constructor)
	Resource::ResourcePtr<Resource::FontResource> m_boldFont;     // optional - see ResolveFont's fallback
	Resource::ResourcePtr<Resource::FontResource> m_italicFont;   // optional
	Resource::ResourcePtr<Resource::FontResource> m_boldItalicFont;  // optional

	Util::Color m_baseColor;
	float m_fontSize;
	TextWrapping m_wrapping = TextWrapping::NONE;
	IconAtlas m_iconAtlas;  // empty (HasTexture() == false) until SetIconAtlas is called - see class comment
	// -1 (the default, matching Godot's RichTextLabel.visible_characters convention) means "show everything".
	// Otherwise caps how many drawable glyphs (see GetRequiredQuadCount's own counting definition - non-
	// space/tab text codepoints plus resolved icon runs, in reading order across the whole label) are actually
	// drawn - see SetVisibleGlyphCount.
	int m_visibleGlyphCount = -1;

	// One handle per animated draw group from the most recent dirty Render() pass (see RenderLine/
	// UIRichLabelAnimGroup) - fully destroyed and rebuilt from scratch every time m_visualDirty triggers a
	// re-render, same "just redo it all" cost model the static path already pays during a reveal animation
	// (see SetVisibleGlyphCount), rather than trying to diff/reuse handles across group-count changes.
	std::vector<Renderer::AnimatedBatchHandle> m_animBatches;

public:
	UIRichLabel(GameContext& context, const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
	           const std::string& markupText, float fontSz = 20.0f);

	// Re-parses immediately (not deferred to Render()) so GetRequiredQuadCount() - which AssignBatches can call
	// before this element's first Render() ever runs - always sees a parse that matches newMarkupText.
	void SetMarkupText(const std::string& newMarkupText);

	void SetFont(Resource::ResourcePtr<Resource::FontResource> newFont)
	{
		m_font        = std::move(newFont);
		m_visualDirty = true;
	}
	void SetBoldFont(Resource::ResourcePtr<Resource::FontResource> newFont)
	{
		m_boldFont    = std::move(newFont);
		m_visualDirty = true;
	}
	void SetItalicFont(Resource::ResourcePtr<Resource::FontResource> newFont)
	{
		m_italicFont  = std::move(newFont);
		m_visualDirty = true;
	}
	void SetBoldItalicFont(Resource::ResourcePtr<Resource::FontResource> newFont)
	{
		m_boldItalicFont = std::move(newFont);
		m_visualDirty    = true;
	}

	// Also re-parses immediately, same reasoning as SetMarkupText - color is baked into each run at parse time
	// (Text::ParseMarkup's baseColor seed), not read live from m_baseColor during Render().
	void SetTextColor(Util::Color color);

	void SetFontSize(float size)
	{
		m_fontSize    = size;
		m_visualDirty = true;
	}
	void SetWordWrap(TextWrapping wrap)
	{
		m_wrapping    = wrap;
		m_visualDirty = true;
	}
	// Copies atlas (its ResourcePtr is refcounted, so this is cheap) - see class comment for how [icon=name]
	// runs resolve against it.
	void SetIconAtlas(const IconAtlas& atlas)
	{
		m_iconAtlas   = atlas;
		m_visualDirty = true;
	}

	// Caps rendering to the first `count` drawable glyphs (see GetRequiredQuadCount) in reading order, for a
	// typewriter/reveal effect - pass a negative value (the default) to show everything. Deliberately a
	// visible-count knob over the FULL markup text rather than a caller truncating the markup string itself
	// frame-by-frame (the approach InteractState.wren's dialogue box currently uses for plain UILabel text) -
	// truncating markup mid-tag (e.g. mid "[color=") is exactly the failure mode this sidesteps, matching
	// Godot's own RichTextLabel.visible_characters/visible_ratio, which exists for the identical reason.
	void SetVisibleGlyphCount(int count)
	{
		m_visibleGlyphCount = count;
		m_visualDirty       = true;
	}
	int GetVisibleGlyphCount() const
	{
		return m_visibleGlyphCount;
	}

	const std::string& GetMarkupText() const
	{
		return m_markupText;
	}
	Util::Color GetTextColor() const
	{
		return m_baseColor;
	}
	float GetFontSize() const
	{
		return m_fontSize;
	}
	TextWrapping GetWordWrap() const
	{
		return m_wrapping;
	}
	// Const-refs, not bools - lets a caller (the editor inspector) distinguish "not set" from "set" without a
	// separate HasXFont() for each, same reasoning as UILabel::GetFont().
	const Resource::ResourcePtr<Resource::FontResource>& GetFont() const
	{
		return m_font;
	}
	const Resource::ResourcePtr<Resource::FontResource>& GetBoldFont() const
	{
		return m_boldFont;
	}
	const Resource::ResourcePtr<Resource::FontResource>& GetItalicFont() const
	{
		return m_italicFont;
	}
	const Resource::ResourcePtr<Resource::FontResource>& GetBoldItalicFont() const
	{
		return m_boldItalicFont;
	}
	bool HasIconAtlas() const
	{
		return m_iconAtlas.HasTexture();
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;
	// Also destroys every handle in m_animBatches (see its own comment) - UIElement::Dispose only knows about
	// m_ownBatch/m_batch, not this class's separate animated-batch handle space.
	void Dispose(GameContext& context) override;

	// One quad per non-space/tab codepoint across every NON-ANIMATED text run's text, plus one per icon run
	// that actually resolves against the current IconAtlas, summed over every line - purely for sizing
	// m_batchSlot (see AssignBatches). Animated runs are deliberately excluded: their quads go into
	// m_animBatches instead (see class comment), never m_batchSlot, so counting them here would over-allocate
	// slot capacity that would never be written to. NOT the same count SetVisibleGlyphCount's cap should be
	// measured against - see GetTotalGlyphCount for that (RenderLine's reveal budget is spent by every run
	// type uniformly, static and animated alike).
	uint32_t GetRequiredQuadCount() const override;

	// Total drawable glyph count across the WHOLE label in reading order - static text, animated text, AND
	// resolved icon runs - matching exactly what RenderLine's remainingVisible reveal budget is actually spent
	// on (see its own comment). This is the count a Wren-side typewriter/reveal loop must drive
	// SetVisibleGlyphCount up to; using GetRequiredQuadCount() for that instead (as this class briefly did)
	// undercounts whenever any animated tag is present, since that function deliberately excludes animated
	// runs for its own (unrelated) m_batchSlot-sizing purpose - the reveal would then cap out before reaching
	// the true end of the string, silently truncating everything after the last counted glyph.
	uint32_t GetTotalGlyphCount() const;

private:
	// tan(~11 degrees) - a typical real italic font's slant angle, applied as a horizontal shear (see
	// UIRenderer::DrawText's shearAmount / FillQuadVertices) when a run needs italics but no dedicated italic
	// FontResource was set (see ResolveFont/RenderLine). Not user-configurable - this is a fallback for when
	// nothing better is available, not a design knob; set a real m_italicFont for actual quality.
	static constexpr float kSyntheticItalicShear = 0.2f;

	void Reparse();

	// Picks the best available font for a run's (bold, italic) combination, falling back progressively toward
	// m_font (always valid) rather than requiring every combination to have been explicitly set - e.g. a
	// bold+italic run falls back to m_boldFont or m_italicFont before finally falling back to m_font, so
	// setting just one variant still visibly does something even before all four are provided.
	//
	// outIsRealItalic reports whether the returned font is actually slanted on disk (m_italicFont or
	// m_boldItalicFont) as opposed to a regular-upright fallback (m_font or m_boldFont) - false whenever
	// `italic` is false too. RenderLine uses this to decide whether DrawText needs a synthetic shear: real
	// italic fonts never get one (they're already slanted), only a run that asked for italics but had to
	// settle for an upright font does. Matches Godot's own stance that a dedicated italic font should always
	// be preferred over a synthesized one when available.
	const Resource::ResourcePtr<Resource::FontResource>& ResolveFont(bool bold, bool italic,
	                                                                 bool& outIsRealItalic) const;

	// m_parsedLines as-is when m_wrapping is NONE (each already a hard line from Text::ParseMarkup's own '\n'
	// splitting); otherwise each of those hard lines gets further soft-wrapped to fit the label's width via
	// TextLayout::Tokenize/WrapTokens, then each wrapped token line is consolidated back into a
	// Text::RichLine (merging adjacent same-run tokens into one RichRun, so an unbroken run of same-style
	// words becomes one DrawText call rather than one per word). Recomputed fresh every dirty Render() pass,
	// same as UILabel::WrapText - not cached separately, since it depends on m_bounds.width which can change
	// independently of any of this class's own setters (UIElement::SetSize already flips m_visualDirty).
	std::vector<Text::RichLine> BuildRenderLines() const;

	// Natural height of one wrapped line: the max over its runs' own natural heights. Every run currently
	// resolves to the same m_fontSize*1.5/IconSize() (no per-run font size exists yet, see class comment), so
	// this is presently a constant in practice - kept as a real max-reduction rather than hardcoding that
	// constant so a future per-run size slots in here without this call site needing to change.
	float ComputeLineHeight(const Text::RichLine& line) const;

	// Side length of a square icon glyph quad - shared by IconSize()'s own callers (icon-token width during
	// wrapping, actual icon quad sizing during render) so an icon's measured width and its drawn width can
	// never disagree. Currently just ComputeLineHeight's own constant - see its comment.
	float IconSize() const
	{
		return m_fontSize * 1.5f;
	}

	// Writes one line's runs left-to-right starting at pos. Non-animated runs go into a sub-slot of
	// m_batchSlot offset by quadOffset quads (same offset-accumulation contract UILabel::RenderGlyphs/
	// RenderJustifiedLine use) - returns how many such quads it actually wrote. Animated runs (effectMask !=
	// 0) instead append AnimQuadVertex data into whichever entry of animGroups matches their (texture,
	// effect+params) - see UIRichLabelAnimGroup - and never touch quadOffset/m_batchSlot at all.
	//
	// remainingVisible is the reveal budget (see SetVisibleGlyphCount), shared and decremented across BOTH
	// paths in the same overall reading order, so a reveal in progress doesn't finish static text before
	// starting animated text (or vice versa) regardless of which comes first in the markup. animCharIndex is
	// a running per-label counter (not reset between lines/runs) that becomes each animated glyph's phase
	// index (see AnimQuadVertex) - both are references threaded across every call for one Render() pass so
	// they carry over correctly from one line to the next.
	uint32_t RenderLine(GameContext& context, const Text::RichLine& line, glm::vec2 pos, uint32_t quadOffset,
	                    int& remainingVisible, int& animCharIndex, std::vector<UIRichLabelAnimGroup>& animGroups);
};
}  // namespace UI
}  // namespace Struktur
