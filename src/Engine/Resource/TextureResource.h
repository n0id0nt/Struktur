#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <string>
#include <vector>

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
public:
	// A decoded image, RGBA8. Decode() touches no shared state (PhysFS reads are thread-safe), so it can run on job
	// workers - see TexturePool::Prefetch.
	struct DecodedImage
	{
		std::vector<uint8_t> pixels;
		int width  = 0;
		int height = 0;
	};
	static bool Decode(const std::string& filePath, DecodedImage& image, std::string& error);
	// Takes an already-decoded image instead of reading the file (LoadFromDisk is then a no-op).
	void SetDecoded(DecodedImage&& image);

private:
	std::vector<uint8_t> m_pixels;  // decoded RGBA8, kept until LoadToGpu() uploads it
	int m_width  = 0;
	int m_height = 0;

public:
	bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;

	TextureResource(const std::string& filePath);
	TextureResource(TextureResource&& other) noexcept;
	TextureResource& operator=(TextureResource&& other) noexcept;

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
