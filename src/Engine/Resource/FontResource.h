#pragma once

#include "Engine/Renderer/RenderTypes.h"

#include <bgfx/bgfx.h>

#include <format>
#include <string>
#include <vector>

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/ResourcePool.h"
#include "Engine/Resource/Resource.h"
#include "Engine/Text/Font.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// Font - GPU resource (contains texture atlas), atlas baked via stb_truetype on both platforms.
class FontResource : public GpuResource
{
private:
	bool m_fontLoaded;
	int m_fontSize;
	int* m_codepoints;  // Custom codepoints for font loading
	int m_codepointCount;
	// Sample the atlas nearest-neighbour instead of bilinear - for pixel-art fonts loaded at their design size (or a
	// multiple) and drawn at whole multiples of it, where filtering would only blur the pixels.
	bool m_pointSampled;
	// The coverage bitmap is kept around until UnloadFromDisk so LoadToGpu can rebuild the bgfx texture after a
	// GPU-context-lost reload without re-parsing the font file (mirrors TextureResource::m_pixels' lifetime).
	bgfx::TextureHandle m_atlasTexture = BGFX_INVALID_HANDLE;
	// An RGBA8 copy of the atlas (white, coverage in alpha) for drawing text through the world's ordinary sprite
	// shader - text particles (SpriteRenderSystem). Built the first time GetWorldAtlas is asked, so fonts only used by
	// the UI never pay for it; freed with the R8 atlas.
	bgfx::TextureHandle m_worldAtlas = BGFX_INVALID_HANDLE;
	std::vector<uint8_t> m_atlasAlpha;
	int m_atlasWidth  = 0;
	int m_atlasHeight = 0;

public:
	Text::Font font;

	FontResource(const std::string& filePath, int size, bool pointSampled = false);
	FontResource(FontResource&& other) noexcept;
	FontResource& operator=(FontResource&& other) noexcept;
	~FontResource();

	bool LoadFromDisk(GameContext& context) override;
	void UnloadFromDisk() override;
	bool LoadToGpu(GameContext& context) override;
	void UnloadFromGpu() override;

	bool IsGpuResourceValid() const override;

	size_t GetMemoryUsage() const override;
	size_t GetGpuMemoryUsage() const override;

	// The atlas as an RGBA8 texture for the world renderer's sprite shader (see m_worldAtlas) - {id, width, height},
	// id 0 / size 0 if it can't be made (nothing loaded yet).
	Renderer::TextureHandle GetWorldAtlas();

	// Font-specific configuration methods
	void SetCodepoints(int* customCodepoints, int count);
	void SetFontSize(int size);
	int GetFontSize();

	int GetBaseSize() const;
	int GetGlyphCount() const;
};
}  // namespace Resource
}  // namespace Struktur
