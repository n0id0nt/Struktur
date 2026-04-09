#include "TextureResource.h"

#include <format>
#include "Engine/Core/FileSystem.h"

Struktur::Resource::TextureResource::TextureResource(const std::string& filePath)
	: GpuResource(filePath)
{
	texture.id = 0;
	m_sourceImage.data = nullptr;
}

Struktur::Resource::TextureResource::~TextureResource()
{
	UnloadFromGpu();
	UnloadFromDisk();
}

bool Struktur::Resource::TextureResource::LoadFromDisk()
{
	if (isLoaded) return true;

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());
	const char* ext = ::GetFileExtension(filePath.c_str());
	m_sourceImage = ::LoadImageFromMemory(ext, result.value.data(), result.value.size());

	if (m_sourceImage.data == nullptr)
	{
		DEBUG_ERROR(std::format("Failed to load image: {}", filePath).c_str());
		return false;
	}

	isLoaded = true;
	DEBUG_INFO(std::format("Loaded texture from disk: {} ({}x{})", filePath, m_sourceImage.width, m_sourceImage.height).c_str());
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

bool Struktur::Resource::TextureResource::LoadToGpu()
{
	if (!LoadFromDisk()) return false;
	if (IsGpuResourceValid()) return true;

	texture = ::LoadTextureFromImage(m_sourceImage);
	return texture.id != 0;
}

void Struktur::Resource::TextureResource::UnloadFromGpu()
{
	if (texture.id != 0)
	{
		::UnloadTexture(texture);
		texture.id = 0;
	}
}

bool Struktur::Resource::TextureResource::IsGpuResourceValid() const
{
	return texture.id != 0/* && IsTextureReady(texture)*/;
}

size_t Struktur::Resource::TextureResource::GetMemoryUsage() const
{
	return isLoaded ? (m_sourceImage.width * m_sourceImage.height * 4) : 0;
}

size_t Struktur::Resource::TextureResource::GetGpuMemoryUsage() const
{
	return GetMemoryUsage(); // Same as disk for simple case
}

Struktur::Resource::TextureResource* Struktur::Resource::TexturePool::LoadResource(const std::string& filePath)
{
	auto* texture = new TextureResource(filePath);

	if (!texture->LoadFromDisk())
	{
		delete texture;
		return nullptr;
	}

	AddGpuResource(texture);
	return texture;
}
