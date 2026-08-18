#include "UIRichLabel.h"

#include <algorithm>
#include <climits>

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Text/Font.h"

namespace
{
// Non-space/tab codepoint count - the same "how many quads would UIRenderer::DrawText actually write for
// this string" definition GetRequiredQuadCount uses, factored out so SetVisibleGlyphCount's per-run reveal
// accounting (see TruncateToVisibleCount below) can share it instead of recounting with its own loop.
int CountVisibleCodepoints(const std::string& text)
{
	int count = 0;
	int index = 0;
	int size  = (int)text.size();
	while (index < size)
	{
		int codepointByteCount = 0;
		int codepoint          = Struktur::Text::GetCodepointNext(&text[index], &codepointByteCount);
		index += codepointByteCount;
		if (codepoint != ' ' && codepoint != '\t')
		{
			++count;
		}
	}
	return count;
}

// Prefix of text containing exactly maxVisible non-space/tab codepoints (or all of it, if it has fewer) - any
// whitespace immediately before the cutoff point is included since it costs no reveal budget itself, matching
// CountVisibleCodepoints' own definition of what counts as a "visible" glyph.
std::string TruncateToVisibleCount(const std::string& text, int maxVisible)
{
	if (maxVisible <= 0)
	{
		return "";
	}
	int visible = 0;
	int index   = 0;
	int size    = (int)text.size();
	while (index < size)
	{
		int codepointByteCount = 0;
		int codepoint          = Struktur::Text::GetCodepointNext(&text[index], &codepointByteCount);
		index += codepointByteCount;
		if (codepoint != ' ' && codepoint != '\t')
		{
			++visible;
		}
		if (visible >= maxVisible)
		{
			break;
		}
	}
	return text.substr(0, index);
}
}  // namespace

Struktur::UI::UIRichLabel::UIRichLabel(GameContext& context, const glm::vec2& absolutePosition,
                                       const glm::vec2& relativePosition, const std::string& markupText,
                                       float fontSz)
    : UIElement(absolutePosition, relativePosition, {0, 0}, {0, 0}),
      m_markupText(markupText),
      m_baseColor(Util::ColorBlack),
      m_fontSize(fontSz)
{
	m_font = context.GetResourceManager().GetFont(context, "default", 32);

	// Rich labels aren't focusable, matching UILabel.
	m_focusable = false;

	Reparse();
}

void Struktur::UI::UIRichLabel::SetMarkupText(const std::string& newMarkupText)
{
	m_markupText = newMarkupText;
	Reparse();
}

void Struktur::UI::UIRichLabel::SetTextColor(Util::Color color)
{
	m_baseColor = color;
	Reparse();
}

void Struktur::UI::UIRichLabel::Reparse()
{
	m_parsedLines = Text::ParseMarkup(m_markupText, m_baseColor);
	m_visualDirty = true;
}

void Struktur::UI::UIRichLabel::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UIRichLabel::Render(GameContext& context)
{
	if (m_visualDirty)
	{
		// Same grow-slot-before-write contract as UILabel::Render - GetRequiredQuadCount() can change whenever
		// the markup text (and therefore m_parsedLines) changes.
		uint32_t requiredQuads = GetRequiredQuadCount();
		if (m_batchSlot.quadCapacity < requiredQuads)
		{
			m_batchSlot = context.GetUIRenderer().AllocateOrResizeSlot(m_batch, m_batchSlot, requiredQuads);
		}

		std::vector<Text::RichLine> renderLines = BuildRenderLines();

		// See SetVisibleGlyphCount - a negative count means "unlimited", represented here as INT_MAX so
		// RenderLine's per-glyph decrement never actually runs out for the unlimited case.
		int remainingVisible = m_visibleGlyphCount < 0 ? INT_MAX : m_visibleGlyphCount;

		glm::vec2 pos      = {m_bounds.x + 5, m_bounds.y + 2.5f};
		uint32_t quadsUsed = 0;
		for (const Text::RichLine& line : renderLines)
		{
			quadsUsed += RenderLine(context, line, pos, quadsUsed, remainingVisible);
			pos.y += ComputeLineHeight(line);
		}

		// Same stale-trailing-quad cleanup UILabel::Render needs - shorter markup than last time must not leave
		// previously-written glyph quads still rendering past what was actually written this time.
		context.GetUIRenderer().ClearSlotFrom(m_batch, m_batchSlot, quadsUsed);

		m_visualDirty = false;
	}

	RenderChildren(context);
}

uint32_t Struktur::UI::UIRichLabel::GetRequiredQuadCount() const
{
	uint32_t quads = 0;
	for (const Text::RichLine& line : m_parsedLines)
	{
		for (const Text::RichRun& run : line.runs)
		{
			if (!run.iconName.empty())
			{
				// Only reserve a quad for an icon run that will actually resolve against the current atlas -
				// RenderLine silently skips drawing anything for one that doesn't (see its own comment), so
				// counting it here too would leave a permanently-unused, wastefully-allocated slot quad.
				Util::Math::Rect unused;
				if (m_iconAtlas.HasTexture() && m_iconAtlas.TryGetIconRect(run.iconName, unused))
				{
					++quads;
				}
				continue;
			}

			quads += (uint32_t)CountVisibleCodepoints(run.text);
		}
	}
	return quads;
}

const Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource>& Struktur::UI::UIRichLabel::ResolveFont(
    bool bold, bool italic, bool& outIsRealItalic) const
{
	outIsRealItalic = false;
	if (bold && italic)
	{
		if (m_boldItalicFont)
		{
			outIsRealItalic = true;
			return m_boldItalicFont;
		}
		if (m_boldFont)
		{
			return m_boldFont;
		}
		if (m_italicFont)
		{
			outIsRealItalic = true;
			return m_italicFont;
		}
		return m_font;
	}
	if (bold)
	{
		return m_boldFont ? m_boldFont : m_font;
	}
	if (italic)
	{
		if (m_italicFont)
		{
			outIsRealItalic = true;
			return m_italicFont;
		}
		return m_font;
	}
	return m_font;
}

std::vector<Struktur::Text::RichLine> Struktur::UI::UIRichLabel::BuildRenderLines() const
{
	if (m_wrapping == TextWrapping::NONE)
	{
		return m_parsedLines;
	}

	float maxWidth = m_bounds.width - 10.0f;  // 5px padding each side, matching UILabel's own convention.
	std::vector<Text::RichLine> result;

	for (const Text::RichLine& line : m_parsedLines)
	{
		// One flat token stream per hard line, spanning every run on it - each run tokenizes itself
		// separately (styleId = that run's index into line.runs) so TextLayout::WrapTokens can wrap across
		// run/style boundaries without needing to know what a "run" or a "font" even is.
		std::vector<TextLayout::WrapToken> tokens;
		for (int runIndex = 0; runIndex < (int)line.runs.size(); ++runIndex)
		{
			const Text::RichRun& run = line.runs[runIndex];
			if (!run.iconName.empty())
			{
				// IconSize() if it'll actually be drawn (matches RenderLine's own resolution check exactly -
				// wrap-width accounting and actual drawn width must never disagree, see IconSize()'s comment),
				// zero-width otherwise so an unresolved icon reference doesn't eat into wrap space for
				// something that won't be visible anyway.
				Util::Math::Rect unused;
				bool resolved = m_iconAtlas.HasTexture() && m_iconAtlas.TryGetIconRect(run.iconName, unused);
				tokens.push_back(TextLayout::WrapToken{"", resolved ? IconSize() : 0.0f, runIndex});
				continue;
			}

			// Shear (see RenderLine) only skews a glyph quad's visual shape, not its measured advance width, so
			// measurement here doesn't need to know whether italics will end up synthesized.
			bool unusedIsRealItalic;
			const Resource::ResourcePtr<Resource::FontResource>& font =
			    ResolveFont(run.bold, run.italic, unusedIsRealItalic);
			auto measure = [this, &font](const std::string& text)
			{ return Text::MeasureTextEx(font->font, text, m_fontSize, 1.0f).x; };
			std::vector<TextLayout::WrapToken> runTokens =
			    TextLayout::Tokenize(run.text, m_wrapping, runIndex, measure);
			tokens.insert(tokens.end(), runTokens.begin(), runTokens.end());
		}

		for (std::vector<TextLayout::WrapToken>& tokenLine : TextLayout::WrapTokens(tokens, maxWidth))
		{
			Text::RichLine outLine;
			for (const TextLayout::WrapToken& token : tokenLine)
			{
				const Text::RichRun& sourceRun = line.runs[token.styleId];
				if (!sourceRun.iconName.empty())
				{
					outLine.runs.push_back(sourceRun);  // Icon runs are never merged/split - copy verbatim.
					continue;
				}

				// Merge into the previous output run when it's the same style (adjacent words that were
				// never actually separated by a style boundary) instead of emitting one RichRun per word -
				// keeps RenderLine's DrawText-per-run count down to one call per contiguous same-style span,
				// matching what Text::ParseMarkup already produces for a line that never needed wrapping.
				if (!outLine.runs.empty() && outLine.runs.back().iconName.empty() &&
				    outLine.runs.back().bold == sourceRun.bold && outLine.runs.back().italic == sourceRun.italic &&
				    outLine.runs.back().color == sourceRun.color)
				{
					outLine.runs.back().text += token.text;
				}
				else
				{
					Text::RichRun newRun = sourceRun;
					newRun.text          = token.text;
					outLine.runs.push_back(std::move(newRun));
				}
			}
			result.push_back(std::move(outLine));
		}
	}

	return result;
}

float Struktur::UI::UIRichLabel::ComputeLineHeight(const Text::RichLine& line) const
{
	// Every run currently resolves to the same natural height (IconSize(), which text runs also share via
	// the same m_fontSize*1.5 constant - see IconSize()'s own comment), so there's nothing to actually take a
	// max over today. This stays its own per-line call (rather than Render() using one constant directly) so
	// real per-run height variation (a future per-run font size) has a single seam to land in later.
	(void)line;
	return IconSize();
}

uint32_t Struktur::UI::UIRichLabel::RenderLine(GameContext& context, const Text::RichLine& line, glm::vec2 pos,
                                               uint32_t quadOffset, int& remainingVisible)
{
	uint32_t quadsWritten = 0;
	float penX            = pos.x;

	for (const Text::RichRun& run : line.runs)
	{
		// Reveal budget exhausted (see SetVisibleGlyphCount) - every remaining run on this and every later
		// line must also stay unrendered, so this deliberately doesn't `break` (a run further down this same
		// line still needs its loop iteration skipped, and Render()'s per-line loop keeps calling RenderLine
		// for later lines regardless - see its own comment).
		if (remainingVisible <= 0)
		{
			continue;
		}

		if (!run.iconName.empty())
		{
			Util::Math::Rect sourceRect;
			if (m_iconAtlas.HasTexture() && m_iconAtlas.TryGetIconRect(run.iconName, sourceRect))
			{
				Resource::TextureResource* iconTexture = m_iconAtlas.GetTexture().Get();
				if (!iconTexture->IsGpuReady())
				{
					iconTexture->LoadToGpu(context);
				}

				float size = IconSize();

				Renderer::UIBatchSlot subSlot = m_batchSlot;
				subSlot.vertexOffset += (quadOffset + quadsWritten) * 4;
				subSlot.quadCapacity -= (quadOffset + quadsWritten);

				// Tinted white, not run.color - icons keep their own source pixel colors (see class comment),
				// unlike text runs which are tinted by the parsed [color=].
				context.GetUIRenderer().DrawTexturedRectRegion(m_batch, subSlot, {penX, pos.y, size, size},
				                                               iconTexture->GetHandle(), sourceRect,
				                                               Util::ColorWhite, GetZIndex());
				++quadsWritten;
				penX += size;
				--remainingVisible;
			}
			// Unresolved (no atlas set, or name not registered in it) - skip entirely, no quad, no pen
			// advance, matching how a run with a style whose font was never set still just falls back rather
			// than erroring (see ResolveFont). Doesn't cost any reveal budget either, for the same reason.
			continue;
		}

		bool isRealItalic = false;
		const Resource::ResourcePtr<Resource::FontResource>& font = ResolveFont(run.bold, run.italic, isRealItalic);

		// Same lazy GPU-upload pattern UILabel::RenderGlyphs uses.
		if (!font->IsGpuReady())
		{
			font->LoadToGpu(context);
		}

		// Only draw as much of this run as the remaining reveal budget allows - CountVisibleCodepoints/
		// TruncateToVisibleCount share the exact same "non-space/tab codepoint" definition
		// GetRequiredQuadCount uses, so a full reveal (remainingVisible >= GetRequiredQuadCount()) always
		// draws every run's full, untruncated text.
		int visibleInRun = CountVisibleCodepoints(run.text);
		const std::string& textToDraw =
		    visibleInRun <= remainingVisible ? run.text : TruncateToVisibleCount(run.text, remainingVisible);

		Renderer::UIBatchSlot subSlot = m_batchSlot;
		subSlot.vertexOffset += (quadOffset + quadsWritten) * 4;
		subSlot.quadCapacity -= (quadOffset + quadsWritten);

		// A run that asked for italics but had to fall back to an upright font gets a synthetic shear instead
		// (see class comment/ResolveFont) - a real italic font never gets one, it's already slanted.
		float shearAmount = (run.italic && !isRealItalic) ? kSyntheticItalicShear : 0.0f;

		quadsWritten += context.GetUIRenderer().DrawText(m_batch, subSlot, font->font, textToDraw, {penX, pos.y},
		                                                 m_fontSize, run.color, GetZIndex(), shearAmount);
		remainingVisible -= std::min(visibleInRun, remainingVisible);

		// No core rendering change needed for multi-run lines - just measure this run's own (possibly
		// truncated) width (GPU-independent, works even before font->LoadToGpu above) and advance the pen
		// before the next run, exactly the loop UILabel::RenderJustifiedLine already does per word.
		penX += Text::MeasureTextEx(font->font, textToDraw, m_fontSize, 1.0f).x;
	}

	return quadsWritten;
}
