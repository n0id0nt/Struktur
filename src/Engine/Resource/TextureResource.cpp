#include "TextureResource.h"

#include <format>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"

#if !defined(PLATFORM_WEB)
	// Implementation lives in bimg (already vendored alongside bgfx); this file only needs the declarations.
	#include <stb_image.h>
#endif

Struktur::Resource::TextureResource::TextureResource(const std::string& filePath)
    : GpuResource(filePath)
{
#if defined(PLATFORM_WEB)
	texture.id         = 0;
	m_sourceImage.data = nullptr;
#endif
}

Struktur::Resource::TextureResource::~TextureResource()
{
	UnloadFromGpu();
	UnloadFromDisk();
}

#if defined(PLATFORM_WEB)
bool Struktur::Resource::TextureResource::LoadFromDisk(GameContext& context)
{
	if (isLoaded)
	{
		return true;
	}

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s, %s", filePath, result.errorMessage.c_str());
	const char* ext = ::GetFileExtension(filePath.c_str());
	m_sourceImage   = ::LoadImageFromMemory(ext, result.value.data(), result.value.size());

	if (m_sourceImage.data == nullptr)
	{
		DEBUG_ERROR(std::format("Failed to load image: {}", filePath).c_str());
		return false;
	}

	isLoaded = true;
	DEBUG_INFO(std::format("Loaded texture from disk: {} ({}x{})", filePath, m_sourceImage.width, m_sourceImage.height)
	               .c_str());
	return true;
}

void Struktur::Resource::TextureResource::UnloadFromDisk()
{
	if (isLoaded)
	{
		::UnloadImage(m_sourceImage);
		m_sourceImage.data = nullptr;
	}
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

	texture = ::LoadTextureFromImage(m_sourceImage);
	if (texture.id != 0)
	{
		gpuState = GpuState::LoadedToGpu;
		return true;
	}
	return false;
}

void Struktur::Resource::TextureResource::UnloadFromGpu()
{
	if (texture.id != 0)
	{
		::UnloadTexture(texture);
		gpuState   = GpuState::Unloaded;
		texture.id = 0;
	}
}

bool Struktur::Resource::TextureResource::IsGpuResourceValid() const
{
	return texture.id != 0;
}

size_t Struktur::Resource::TextureResource::GetMemoryUsage() const
{
	return isLoaded ? (m_sourceImage.width * m_sourceImage.height * 4) : 0;
}
#else
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
	texture = bgfx::createTexture2D((uint16_t)m_width, (uint16_t)m_height, false, 1, bgfx::TextureFormat::RGBA8,
	                                 0, memory);
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
#endif

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
