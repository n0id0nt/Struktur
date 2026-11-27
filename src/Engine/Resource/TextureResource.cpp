#include "TextureResource.h"

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

Struktur::Resource::TextureResource::TextureResource(TextureResource&& other) noexcept
    : GpuResource(std::move(other)),
      m_pixels(std::move(other.m_pixels)),
      m_width(other.m_width),
      m_height(other.m_height),
      texture(other.texture)
{
	other.texture  = BGFX_INVALID_HANDLE;
	other.m_width  = 0;
	other.m_height = 0;
}

Struktur::Resource::TextureResource& Struktur::Resource::TextureResource::operator=(TextureResource&& other) noexcept
{
	if (this != &other)
	{
		UnloadFromGpu();
		UnloadFromDisk();
		GpuResource::operator=(std::move(other));
		m_pixels       = std::move(other.m_pixels);
		m_width        = other.m_width;
		m_height       = other.m_height;
		texture        = other.texture;
		other.texture  = BGFX_INVALID_HANDLE;
		other.m_width  = 0;
		other.m_height = 0;
	}
	return *this;
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

	DecodedImage image;
	std::string error;
	if (!Decode(filePath, image, error))
	{
		DEBUG_ERROR("Failed to load image: %s (%s)", filePath, error.c_str());
		return false;
	}
	SetDecoded(std::move(image));
	DEBUG_INFO("Loaded texture from disk: %s (%dx%d)", filePath, m_width, m_height);
	return true;
}

bool Struktur::Resource::TextureResource::Decode(const std::string& filePath, DecodedImage& image, std::string& error)
{
	auto result = FileSystem::ReadBytes(filePath);
	if (!result.success)
	{
		error = result.errorMessage;
		return false;
	}
	int channels    = 0;
	stbi_uc* pixels = ::stbi_load_from_memory(result.value.data(), (int)result.value.size(), &image.width,
	                                          &image.height, &channels, 4);
	if (!pixels)
	{
		error = "not a readable image";
		return false;
	}
	image.pixels.assign(pixels, pixels + (size_t)image.width * image.height * 4);
	::stbi_image_free(pixels);
	return true;
}

void Struktur::Resource::TextureResource::SetDecoded(DecodedImage&& image)
{
	m_pixels = std::move(image.pixels);
	m_width  = image.width;
	m_height = image.height;
	isLoaded = true;
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
