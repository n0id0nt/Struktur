#include "FontResource.h"

#include <algorithm>
#include <cmath>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"

Struktur::Resource::FontResource::FontResource(const std::string& filePath, int size)
    : GpuResource(filePath),
      m_fontLoaded(false),
      m_fontSize(size),
      m_codepoints(nullptr),
      m_codepointCount(0)
{
	font.baseSize = 0;
	font.glyphs.clear();
	font.texture = Text::TextureRef{};
}

Struktur::Resource::FontResource::FontResource(FontResource&& other) noexcept
    : GpuResource(std::move(other)),
      m_fontLoaded(other.m_fontLoaded),
      m_fontSize(other.m_fontSize),
      m_codepoints(other.m_codepoints),
      m_codepointCount(other.m_codepointCount),
      m_atlasTexture(other.m_atlasTexture),
      m_atlasAlpha(std::move(other.m_atlasAlpha)),
      m_atlasWidth(other.m_atlasWidth),
      m_atlasHeight(other.m_atlasHeight),
      font(std::move(other.font))
{
	other.m_fontLoaded   = false;
	other.m_atlasTexture = BGFX_INVALID_HANDLE;
}

Struktur::Resource::FontResource& Struktur::Resource::FontResource::operator=(FontResource&& other) noexcept
{
	if (this != &other)
	{
		UnloadFromGpu();
		UnloadFromDisk();
		GpuResource::operator=(std::move(other));
		m_fontLoaded         = other.m_fontLoaded;
		m_fontSize           = other.m_fontSize;
		m_codepoints         = other.m_codepoints;
		m_codepointCount     = other.m_codepointCount;
		m_atlasTexture       = other.m_atlasTexture;
		m_atlasAlpha         = std::move(other.m_atlasAlpha);
		m_atlasWidth         = other.m_atlasWidth;
		m_atlasHeight        = other.m_atlasHeight;
		font                 = std::move(other.font);
		other.m_fontLoaded   = false;
		other.m_atlasTexture = BGFX_INVALID_HANDLE;
	}
	return *this;
}

Struktur::Resource::FontResource::~FontResource()
{
	UnloadFromGpu();
	UnloadFromDisk();
}

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace
{
// raylib's built-in bitmap font (the old "default" shortcut) has no equivalent without a real file - point the
// same shortcut at an already-shipped ttf instead. Invisible to callers either way.
constexpr const char* kDefaultFontPath = "Fonts/machine-std/machine-std-regular.ttf";
// Starting size for the pack-and-retry loop below, not a hard limit - a fixed atlas size that happened to fit
// this project's original, smaller font sizes silently dropped glyphs (as zero-size, invisible rects) the moment
// a larger fontSize/codepoint-count combination needed more area than one atlas this size could hold.
constexpr int kInitialAtlasSize = 512;
// Doubled from kInitialAtlasSize on each overflow until packing succeeds or this cap is hit - 4096 comfortably
// fits every font size this project actually uses (see LoadFromDisk) while staying well within any GPU's texture
// size limits.
constexpr int kMaxAtlasSize = 4096;
constexpr int kAtlasPadding = 1;
}  // namespace

bool Struktur::Resource::FontResource::LoadFromDisk(GameContext& context)
{
	if (m_fontLoaded)
	{
		return true;
	}

	const std::string& path = (filePath.empty() || filePath == "default") ? kDefaultFontPath : filePath;
	auto result             = FileSystem::ReadBytes(path);
	ASSERT_MSG(result.success, "Failed to load font: %s", result.errorMessage.c_str());

	stbtt_fontinfo fontInfo{};
	if (!stbtt_InitFont(&fontInfo, result.value.data(), 0))
	{
		BREAK_MSG("Failed to parse font: %s", path);
		return false;
	}

	int ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
	float scale        = stbtt_ScaleForPixelHeight(&fontInfo, (float)m_fontSize);
	float ascentPixels = (float)ascent * scale;
	int codepointCount = (m_codepoints != nullptr && m_codepointCount > 0) ? m_codepointCount : 95;

	std::vector<stbtt_packedchar> packedChars(codepointCount);

	// Retry at double the size on overflow rather than failing once at a fixed size - stbtt_PackFontRanges only
	// fills chardata_for_range entries it actually manages to place, so a failed attempt can leave some entries
	// holding coordinates from a smaller, no-longer-relevant atlas size; re-zeroing before each attempt means a
	// glyph that still doesn't fit even at kMaxAtlasSize ends up with a clean zero-size (invisible) rect instead
	// of stale, wrongly-offset ones pointing at the wrong spot in the final atlas.
	int packOk = 0;
	for (int atlasSize = kInitialAtlasSize; atlasSize <= kMaxAtlasSize; atlasSize *= 2)
	{
		m_atlasWidth  = atlasSize;
		m_atlasHeight = atlasSize;
		m_atlasAlpha.assign((size_t)m_atlasWidth * (size_t)m_atlasHeight, 0);
		std::fill(packedChars.begin(), packedChars.end(), stbtt_packedchar{});

		stbtt_pack_context packContext{};
		stbtt_PackBegin(&packContext, m_atlasAlpha.data(), m_atlasWidth, m_atlasHeight, 0, kAtlasPadding, nullptr);

		stbtt_pack_range range{};
		range.font_size          = (float)m_fontSize;
		range.num_chars          = codepointCount;
		range.chardata_for_range = packedChars.data();
		if (m_codepoints != nullptr && m_codepointCount > 0)
		{
			range.array_of_unicode_codepoints = m_codepoints;
		}
		else
		{
			// Matches raylib's default 95-char ASCII range (32-126, includes the '?' fallback glyph at 63).
			range.first_unicode_codepoint_in_range = 32;
		}

		packOk = stbtt_PackFontRanges(&packContext, result.value.data(), 0, &range, 1);
		stbtt_PackEnd(&packContext);
		if (packOk)
		{
			break;
		}
	}

	if (!packOk)
	{
		DEBUG_WARNING("Font atlas overflowed for %s even at the max %dx%d size - some glyphs may be missing", path,
		             kMaxAtlasSize, kMaxAtlasSize);
	}

	font.baseSize = m_fontSize;
	font.glyphs.assign(codepointCount, Text::Glyph{});

	for (int i = 0; i < codepointCount; ++i)
	{
		const stbtt_packedchar& packedChar = packedChars[i];
		int codepoint = (m_codepoints != nullptr && m_codepointCount > 0) ? m_codepoints[i] : (32 + i);

		Text::Glyph& glyph = font.glyphs[i];
		glyph.value        = codepoint;
		glyph.offsetX      = (int)std::round(packedChar.xoff);
		// stb's yoff is baseline-relative; raylib's convention is relative to the line's top (see raylib's
		// rtext.c LoadFontData: offsetY = stbGlyphBoxTop + ascent*scale) - replicate the same shift here so
		// glyphs from this atlas line up the same way raylib's own would.
		glyph.offsetY  = (int)std::round(packedChar.yoff + ascentPixels);
		glyph.advanceX = (int)std::round(packedChar.xadvance);
		glyph.rec = Util::Math::Rect{(float)packedChar.x0, (float)packedChar.y0, (float)(packedChar.x1 - packedChar.x0),
		                             (float)(packedChar.y1 - packedChar.y0)};
	}

	m_fontLoaded = true;
	isLoaded     = true;
	DEBUG_INFO("Loaded font from disk: %s (size: %d, glyphs: %d)", path, m_fontSize, codepointCount);
	return true;
}

void Struktur::Resource::FontResource::UnloadFromDisk()
{
	font.glyphs.clear();
	font.baseSize = 0;
	m_atlasAlpha.clear();
	m_atlasAlpha.shrink_to_fit();
	m_fontLoaded = false;
	isLoaded     = false;
}

bool Struktur::Resource::FontResource::LoadToGpu(GameContext& context)
{
	if (!isLoaded)
	{
		return false;
	}
	if (bgfx::isValid(m_atlasTexture))
	{
		gpuState = GpuState::LoadedToGpu;
		return true;
	}

	// Uploaded as-is (one byte of coverage per texel) rather than expanded to RGBA8 - a quarter the GPU memory
	// of the old {255,255,255,coverage} expansion. fs_spriteCoverage.sc (UIRenderer::Flush picks it for any run
	// whose quads are tagged coverage=true, see UIBatch::quadIsCoverage) broadcasts the sampled .r channel into
	// rgba and tints via vertex color, in place of fs_sprite.sc's texture.rgba * vertexColor.rgba.
	const bgfx::Memory* memory = bgfx::copy(m_atlasAlpha.data(), (uint32_t)m_atlasAlpha.size());
	m_atlasTexture             = bgfx::createTexture2D((uint16_t)m_atlasWidth, (uint16_t)m_atlasHeight, false, 1,
	                                                   bgfx::TextureFormat::R8, 0, memory);
	if (!bgfx::isValid(m_atlasTexture))
	{
		return false;
	}

	// Same "id doubles as the bgfx handle's idx" convention TextureResource::GetHandle() already uses on desktop.
	font.texture.id     = (unsigned int)m_atlasTexture.idx;
	font.texture.width  = m_atlasWidth;
	font.texture.height = m_atlasHeight;
	gpuState            = GpuState::LoadedToGpu;
	return true;
}

void Struktur::Resource::FontResource::UnloadFromGpu()
{
	if (bgfx::isValid(m_atlasTexture))
	{
		bgfx::destroy(m_atlasTexture);
		m_atlasTexture  = BGFX_INVALID_HANDLE;
		font.texture.id = 0;
	}
	m_fontLoaded = false;
}

bool Struktur::Resource::FontResource::IsGpuResourceValid() const
{
	// bgfx handle index 0 is a *valid* handle, not a sentinel - font.texture.id alone can't be trusted here.
	return bgfx::isValid(m_atlasTexture) && font.baseSize > 0;
}

size_t Struktur::Resource::FontResource::GetMemoryUsage() const
{
	if (!m_fontLoaded)
	{
		return 0;
	}

	// Estimate: glyph data + texture data
	size_t glyphDataSize = font.glyphs.size() * sizeof(Text::Glyph);
	size_t textureSize   = (size_t)font.texture.width * (size_t)font.texture.height;  // R8, 1 byte/texel
	return glyphDataSize + textureSize;
}

size_t Struktur::Resource::FontResource::GetGpuMemoryUsage() const
{
	// R8, 1 byte/texel (see LoadToGpu).
	return m_fontLoaded ? ((size_t)font.texture.width * (size_t)font.texture.height) : 0;
}

void Struktur::Resource::FontResource::SetCodepoints(int* customCodepoints, int count)
{
	m_codepoints     = customCodepoints;
	m_codepointCount = count;
}

void Struktur::Resource::FontResource::SetFontSize(int size)
{
	m_fontSize = size;
}

int Struktur::Resource::FontResource::GetFontSize()
{
	return m_fontSize;
}

int Struktur::Resource::FontResource::GetBaseSize() const
{
	return font.baseSize;
}

int Struktur::Resource::FontResource::GetGlyphCount() const
{
	return (int)font.glyphs.size();
}
