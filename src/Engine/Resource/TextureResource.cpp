#include "TextureResource.h"

#include <format>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"

// bimg vendors stb_image's header but its own STB_IMAGE_IMPLEMENTATION lives in bimg_decode, a much heavier
// target (pulls in AVIF/dav1d/libavif) that's otherwise unused here - compile the implementation directly
// instead, same pattern as FontResource.cpp's STB_TRUETYPE_IMPLEMENTATION.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Struktur::Resource::TextureResource::TextureResource(const std::string& filePath)
    : GpuResource(filePath)
{
}

Struktur::Resource::TextureResource::~TextureResource()
{
	UnloadFromGpu();
	UnloadFromDisk();
}

bool Struktur::Resource::TextureResource::LoadFromDisk(GameContext& context)
{
	if (isLoaded)
	{
		return true;
	}

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s, %s", filePath, result.errorMessage.c_str());

	int channels = 0;
	stbi_uc* pixels =
	    ::stbi_load_from_memory(result.value.data(), (int)result.value.size(), &m_width, &m_height, &channels, 4);
	if (!pixels)
	{
		DEBUG_ERROR(std::format("Failed to load image: {}", filePath).c_str());
		return false;
	}

	m_pixels.assign(pixels, pixels + (size_t)m_width * m_height * 4);
	::stbi_image_free(pixels);

	isLoaded = true;
	DEBUG_INFO(std::format("Loaded texture from disk: {} ({}x{})", filePath, m_width, m_height).c_str());
	return true;
}

void Struktur::Resource::TextureResource::UnloadFromDisk()
{
	m_pixels.clear();
	m_pixels.shrink_to_fit();
	isLoaded = false;
}

bool Struktur::Resource::TextureResource::LoadToGpu(GameContext& context)
{
	if (!isLoaded)
	{
		return false;
	}
	if (IsGpuResourceValid())
	{
		gpuState = GpuState::LoadedToGpu;
		return true;
	}

	const bgfx::Memory* memory = bgfx::copy(m_pixels.data(), (uint32_t)m_pixels.size());
	// Point (nearest-neighbor) sampling instead of bgfx's default bilinear - keeps pixel art crisp instead of
	// blurring it when sprites/tiles are scaled or land on non-integer pixel boundaries.
	texture = bgfx::createTexture2D((uint16_t)m_width, (uint16_t)m_height, false, 1, bgfx::TextureFormat::RGBA8,
	                                 BGFX_SAMPLER_POINT, memory);
	if (bgfx::isValid(texture))
	{
		gpuState = GpuState::LoadedToGpu;
		return true;
	}
	return false;
}

void Struktur::Resource::TextureResource::UnloadFromGpu()
{
	if (bgfx::isValid(texture))
	{
		bgfx::destroy(texture);
		texture  = BGFX_INVALID_HANDLE;
		gpuState = GpuState::Unloaded;
	}
}

bool Struktur::Resource::TextureResource::IsGpuResourceValid() const
{
	return bgfx::isValid(texture);
}

size_t Struktur::Resource::TextureResource::GetMemoryUsage() const
{
	return isLoaded ? m_pixels.size() : 0;
}

size_t Struktur::Resource::TextureResource::GetGpuMemoryUsage() const
{
	return GetMemoryUsage();  // Same as disk for simple case
}

Struktur::Resource::TextureResource* Struktur::Resource::TexturePool::LoadResource(GameContext& context,
                                                                                   const std::string& filePath)
{
	auto* texture = new TextureResource(filePath);

	if (!texture->LoadFromDisk(context))
	{
		delete texture;
		return nullptr;
	}

	AddGpuResource(texture);
	return texture;
}
