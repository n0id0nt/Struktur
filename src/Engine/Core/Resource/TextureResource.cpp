#include "TextureResource.h"

#include <format>

Struktur::Core::Resource::TextureResource::TextureResource(const std::string &filePath)
: GpuResource(filePath)
{
    texture.id = 0;
    m_sourceImage.data = nullptr;
}

Struktur::Core::Resource::TextureResource::~TextureResource()
{
    UnloadFromGpu();
    UnloadFromDisk();
}

bool Struktur::Core::Resource::TextureResource::LoadFromDisk()
{
    if (isLoaded) return true;
    
    m_sourceImage = ::LoadImage(filePath.c_str());
    if (m_sourceImage.data == nullptr)
    {
        DEBUG_ERROR(std::format("Failed to load image: {}", filePath).c_str());
        return false;
    }
    
    isLoaded = true;
    DEBUG_INFO(std::format("Loaded texture from disk: {} ({}x{})", filePath, m_sourceImage.width, m_sourceImage.height).c_str());
    return true;
}

void Struktur::Core::Resource::TextureResource::UnloadFromDisk()
{
    if (isLoaded)
    {
		::UnloadImage(m_sourceImage);
        m_sourceImage.data = nullptr;
    }
    isLoaded = false;
}

bool Struktur::Core::Resource::TextureResource::LoadToGpu()
{
    if (!LoadFromDisk()) return false;
    if (IsGpuResourceValid()) return true;
    
    texture = ::LoadTextureFromImage(m_sourceImage);
    return texture.id != 0;
}

void Struktur::Core::Resource::TextureResource::UnloadFromGpu()
{
    if (texture.id != 0)
    {
        ::UnloadTexture(texture);
        texture.id = 0;
    }
}

bool Struktur::Core::Resource::TextureResource::IsGpuResourceValid() const
{
    return texture.id != 0/* && IsTextureReady(texture)*/;
}

size_t Struktur::Core::Resource::TextureResource::GetMemoryUsage() const
{
    return isLoaded ? (m_sourceImage.width * m_sourceImage.height * 4) : 0;
}

size_t Struktur::Core::Resource::TextureResource::GetGpuMemoryUsage() const
{
    return GetMemoryUsage(); // Same as disk for simple case
}

Struktur::Core::Resource::TextureResource *Struktur::Core::Resource::TexturePool::LoadResource(const std::string& filePath)
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
