#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"

#if defined(PLATFORM_WEB)
	#include "raylib.h"
#else
	#include <bgfx/bgfx.h>
#endif

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// GPU texture - bgfx on desktop, raylib on web (see LoadToGpu/UnloadFromGpu).
class TextureResource : public GpuResource
{
   private:
#if defined(PLATFORM_WEB)
	::Image m_sourceImage;
#else
	std::vector<uint8_t> m_pixels;  // decoded RGBA8, kept until LoadToGpu() uploads it
	int m_width  = 0;
	int m_height = 0;
#endif

   public:
#if defined(PLATFORM_WEB)
	::Texture2D texture;
#else
	bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
#endif

	TextureResource(const std::string& filePath);

	~TextureResource();

	bool LoadFromDisk(GameContext& context) override;
	void UnloadFromDisk() override;
	bool LoadToGpu(GameContext& context) override;
	void UnloadFromGpu() override;
	bool IsGpuResourceValid() const override;
	size_t GetMemoryUsage() const override;
	size_t GetGpuMemoryUsage() const override;
	int GetWidth() const
	{
#if defined(PLATFORM_WEB)
		return m_sourceImage.width;
#else
		return m_width;
#endif
	}
	int GetHeight() const
	{
#if defined(PLATFORM_WEB)
		return m_sourceImage.height;
#else
		return m_height;
#endif
	}
	Renderer::TextureHandle GetHandle() const
	{
#if defined(PLATFORM_WEB)
		return Renderer::TextureHandle{texture.id, texture.width, texture.height};
#else
		return Renderer::TextureHandle{texture.idx, m_width, m_height};
#endif
	}
};

// Specialized pools
class TexturePool : public GpuResourcePool<TextureResource>
{
   public:
	TexturePool()
	    : GpuResourcePool<TextureResource>(256 * 1024 * 1024)
	{
	}  // 256MB for textures

   protected:
	TextureResource* LoadResource(GameContext& context, const std::string& filePath) override;
};
}  // namespace Resource
}  // namespace Struktur
