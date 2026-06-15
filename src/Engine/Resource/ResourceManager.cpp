#include "ResourceManager.h"

Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource> Struktur::Resource::ResourceManager::GetTexture(
    const std::string& filePath)
{
	return m_texturePool.GetResource(filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource> Struktur::Resource::ResourceManager::GetSound(
    const std::string& filePath)
{
	return m_soundPool.GetResource(filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource> Struktur::Resource::ResourceManager::GetMusic(
    const std::string& filePath)
{
	return m_musicPool.GetResource(filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource> Struktur::Resource::ResourceManager::GetFont(
    const std::string& filePath, int size)
{
	return m_fontPool.GetResource(std::format("{}_{}", filePath, size));
}

Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource> Struktur::Resource::ResourceManager::GetShader(
    const std::string& vsFilePath, const std::string& fsFilePath)
{
	return m_shaderPool.GetResource(std::format("{},{}", vsFilePath, fsFilePath));
}

void Struktur::Resource::ResourceManager::Clear()
{
	m_texturePool.Clear();
	m_soundPool.Clear();
	m_musicPool.Clear();
	m_fontPool.Clear();
	m_shaderPool.Clear();
}

void Struktur::Resource::ResourceManager::HandleGpuContextLost()
{
	DEBUG_INFO("=== GPU CONTEXT LOST ===");
	m_texturePool.HandleGpuContextLost();
	m_fontPool.HandleGpuContextLost();
	m_shaderPool.HandleGpuContextLost();
	// Note: Sound and music pools are unaffected
}

void Struktur::Resource::ResourceManager::ReloadAllGpuResources()
{
	DEBUG_INFO("=== RELOADING GPU RESOURCES ===");
	m_texturePool.ReloadAllGpuResources();
	m_fontPool.ReloadAllGpuResources();
	m_shaderPool.ReloadAllGpuResources();
	// Note: Sound and music pools are unaffected
}

void Struktur::Resource::ResourceManager::PrintResourceStats() const
{
	DEBUG_INFO(
	    "=== Resource Statistics ===\n\nGPU Resources:\n  Texture: %d\n  GPU Memory: %dMB / %dMB (%d%)\n  System "
	    "Memory: %dMB\n\nCPU Resources:\n  Sounds: %d (%dMB)\n  Music: %d (%dMB)",
	    m_texturePool.GetLoadedCount(), m_texturePool.GetGpuMemoryUsage(), m_texturePool.GetMaxGpuMemory(),
	    m_texturePool.GetGpuMemoryUsagePercent(), m_texturePool.GetTotalMemoryUsage(), m_soundPool.GetLoadedCount(),
	    m_soundPool.GetTotalMemoryUsage(), m_musicPool.GetLoadedCount(), m_musicPool.GetTotalMemoryUsage());
}
