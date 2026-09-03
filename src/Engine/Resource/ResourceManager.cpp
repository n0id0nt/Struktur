#include "ResourceManager.h"

Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource> Struktur::Resource::ResourceManager::GetTexture(
    GameContext& context, const std::string& filePath)
{
	return m_texturePool.GetResource(context, filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource> Struktur::Resource::ResourceManager::GetSound(
    GameContext& context, const std::string& filePath)
{
	return m_soundPool.GetResource(context, filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource> Struktur::Resource::ResourceManager::GetMusic(
    GameContext& context, const std::string& filePath)
{
	return m_musicPool.GetResource(context, filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource> Struktur::Resource::ResourceManager::GetFont(
    GameContext& context, const std::string& filePath, int size)
{
	return m_fontPool.GetResource(context, FontKey{filePath, size});
}

Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource> Struktur::Resource::ResourceManager::GetShader(
    GameContext& context, const std::string& vsFilePath, const std::string& fsFilePath)
{
	return m_shaderPool.GetResource(context, ShaderKey{vsFilePath, fsFilePath});
}

#ifdef EDITOR
Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource>
Struktur::Resource::ResourceManager::GetEditorTexture(GameContext& context, const std::string& filePath)
{
	return m_editorTexturePool.GetResource(context, filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource>
Struktur::Resource::ResourceManager::GetEditorSound(GameContext& context, const std::string& filePath)
{
	return m_editorSoundPool.GetResource(context, filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource>
Struktur::Resource::ResourceManager::GetEditorMusic(GameContext& context, const std::string& filePath)
{
	return m_editorMusicPool.GetResource(context, filePath);
}

Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource>
Struktur::Resource::ResourceManager::GetEditorFont(GameContext& context, const std::string& filePath, int size)
{
	return m_editorFontPool.GetResource(context, FontKey{filePath, size});
}

Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource>
Struktur::Resource::ResourceManager::GetEditorShader(GameContext& context, const std::string& vsFilePath,
                                                     const std::string& fsFilePath)
{
	return m_editorShaderPool.GetResource(context, ShaderKey{vsFilePath, fsFilePath});
}
#endif

void Struktur::Resource::ResourceManager::ClearGameResources()
{
	m_texturePool.Clear();
	m_soundPool.Clear();
	m_musicPool.Clear();
	m_fontPool.Clear();
	m_shaderPool.Clear();
}

void Struktur::Resource::ResourceManager::Clear()
{
	ClearGameResources();
#ifdef EDITOR
	m_editorTexturePool.Clear();
	m_editorSoundPool.Clear();
	m_editorMusicPool.Clear();
	m_editorFontPool.Clear();
	m_editorShaderPool.Clear();
#endif
}

void Struktur::Resource::ResourceManager::HandleGpuContextLost()
{
	DEBUG_INFO("=== GPU CONTEXT LOST ===");
	m_texturePool.HandleGpuContextLost();
	m_fontPool.HandleGpuContextLost();
	m_shaderPool.HandleGpuContextLost();
	// Note: Sound and music pools are unaffected
}

void Struktur::Resource::ResourceManager::ReloadAllGpuResources(GameContext& context)
{
	DEBUG_INFO("=== RELOADING GPU RESOURCES ===");
	m_texturePool.ReloadAllGpuResources(context);
	m_fontPool.ReloadAllGpuResources(context);
	m_shaderPool.ReloadAllGpuResources(context);
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
