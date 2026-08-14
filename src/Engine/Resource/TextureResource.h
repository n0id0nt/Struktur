#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>

#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/ResourcePool.h"
#include "Engine/Resource/Resource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
class TextureResource : public GpuResource
{
   private:
	std::vector<uint8_t> m_pixels;  // decoded RGBA8, kept until LoadToGpu() uploads it
	int m_width  = 0;
	int m_height = 0;

   public:
	bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;

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
		return m_width;
	}
	int GetHeight() const
	{
		return m_height;
	}
	Renderer::TextureHandle GetHandle() const
	{
		return Renderer::TextureHandle{texture.idx, m_width, m_height};
	}
};
}  // namespace Resource
}  // namespace Struktur
