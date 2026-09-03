#pragma once

#include <string>

#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/MusicResource.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/FontPool.h"
#include "Engine/Resource/Pools/MusicPool.h"
#include "Engine/Resource/Pools/ShaderPool.h"
#include "Engine/Resource/Pools/SoundPool.h"
#include "Engine/Resource/Pools/TexturePool.h"
#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourceEvents.h"
#include "Engine/Resource/ShaderResource.h"
#include "Engine/Resource/SoundResource.h"
#include "Engine/Resource/TextureResource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// Enhanced resource manager
class ResourceManager
{
private:
	TexturePool m_texturePool;
	SoundPool m_soundPool;
	MusicPool m_musicPool;
	FontPool m_fontPool;
	ShaderPool m_shaderPool;

#ifdef EDITOR
	// Editor-only resource pools. Anything the editor itself needs to keep resident across a game
	// reset (file-explorer thumbnails, preview-window textures, ...) is fetched through GetEditor*()
	// below and lives here instead of in the game pools above. Two consequences:
	//   - ClearGameResources() (called on every game reset/restart) wipes only the game pools, so
	//     editor-held ResourcePtrs stay valid across the reset.
	//   - The "every game ResourcePtr was released" assert in ClearGameSystems() only inspects the
	//     game pools, so editor-held resources never trip it.
	// Only Clear() (full engine teardown) empties these.
	TexturePool m_editorTexturePool;
	SoundPool m_editorSoundPool;
	MusicPool m_editorMusicPool;
	FontPool m_editorFontPool;
	ShaderPool m_editorShaderPool;
#endif

public:
	ResourcePtr<TextureResource> GetTexture(GameContext& context, const std::string& filePath);
	ResourcePtr<SoundResource> GetSound(GameContext& context, const std::string& filePath);
	ResourcePtr<MusicResource> GetMusic(GameContext& context, const std::string& filePath);
	ResourcePtr<FontResource> GetFont(GameContext& context, const std::string& filePath, int size);
	ResourcePtr<ShaderResource> GetShader(GameContext& context, const std::string& vsFilePath,
	                                      const std::string& fsFilePath);

#ifdef EDITOR
	// Editor-only counterparts of the getters above - resources fetched here are held in the editor
	// pools and are exempt from the game-reset clear/assert (see the m_editor*Pool comment above).
	// Use these from editor windows/preview renderers; never from gameplay/script code.
	ResourcePtr<TextureResource> GetEditorTexture(GameContext& context, const std::string& filePath);
	ResourcePtr<SoundResource> GetEditorSound(GameContext& context, const std::string& filePath);
	ResourcePtr<MusicResource> GetEditorMusic(GameContext& context, const std::string& filePath);
	ResourcePtr<FontResource> GetEditorFont(GameContext& context, const std::string& filePath, int size);
	ResourcePtr<ShaderResource> GetEditorShader(GameContext& context, const std::string& vsFilePath,
	                                            const std::string& fsFilePath);
#endif

	const TexturePool& GetTexturePool()
	{
		return m_texturePool;
	}
	const SoundPool& GetSoundPool()
	{
		return m_soundPool;
	}
	const MusicPool& GetMusicPool()
	{
		return m_musicPool;
	}
	const FontPool& GetFontPool()
	{
		return m_fontPool;
	}
	const ShaderPool& GetShaderPool()
	{
		return m_shaderPool;
	}

#ifdef EDITOR
	const TexturePool& GetEditorTexturePool()
	{
		return m_editorTexturePool;
	}
	const SoundPool& GetEditorSoundPool()
	{
		return m_editorSoundPool;
	}
	const MusicPool& GetEditorMusicPool()
	{
		return m_editorMusicPool;
	}
	const FontPool& GetEditorFontPool()
	{
		return m_editorFontPool;
	}
	const ShaderPool& GetEditorShaderPool()
	{
		return m_editorShaderPool;
	}
#endif

	// Clears only the game resource pools. Called on every game reset/restart (ClearGameSystems) -
	// the editor pools, if present, are deliberately left untouched so editor-held ResourcePtrs
	// survive the reset.
	void ClearGameResources();

	// Clears every pool, game and (in editor builds) editor. Full engine teardown only.
	void Clear();

	// GPU-specific operations (only affect GPU resources)
	void HandleGpuContextLost();
	void ReloadAllGpuResources(GameContext& context);

	void PrintResourceStats() const;

#ifdef EDITOR
	// Registers ONE callback that receives Loaded/ReadyForUse/Unloaded events from every pool, each tagged with
	// its own ResourceCategory - the resource manager editor window (ResourceManagerWindow) calls this once
	// from Initialise() rather than reaching into each pool individually. Each pool itself has no notion of its
	// own category (see PoolResourceEventCallback's comment in ResourceEvents.h), so this is where that tag
	// actually gets attached, one small forwarding lambda per pool.
	void SetResourceEventCallback(ResourceEventCallback cb)
	{
		m_texturePool.SetResourceEventCallback(
		    [cb](ResourceEventType e, const std::string& n, size_t m, double t)
		    { cb(ResourceCategory::Texture, e, n, m, t); });
		m_soundPool.SetResourceEventCallback(
		    [cb](ResourceEventType e, const std::string& n, size_t m, double t)
		    { cb(ResourceCategory::Sound, e, n, m, t); });
		m_musicPool.SetResourceEventCallback(
		    [cb](ResourceEventType e, const std::string& n, size_t m, double t)
		    { cb(ResourceCategory::Music, e, n, m, t); });
		m_fontPool.SetResourceEventCallback(
		    [cb](ResourceEventType e, const std::string& n, size_t m, double t)
		    { cb(ResourceCategory::Font, e, n, m, t); });
		m_shaderPool.SetResourceEventCallback(
		    [cb](ResourceEventType e, const std::string& n, size_t m, double t)
		    { cb(ResourceCategory::Shader, e, n, m, t); });
	}
#endif
};
}  // namespace Resource
}  // namespace Struktur
// Usage examples:
/*
// Example usage
int main() {
    std::cout << "=== Raylib GPU/Non-GPU Resource Pool Demo ===\n\n";

    GameResourceManager resourceManager;

    std::cout << "1. Loading mixed resources:\n";
    auto playerTexture = resourceManager.getTexture("player");     // GPU resource
    auto enemyTexture = resourceManager.getTexture("enemy");       // GPU resource
    auto jumpSound = resourceManager.getSound("jump");             // Non-GPU resource
    auto backgroundMusic = resourceManager.getMusic("background"); // Non-GPU resource

    std::cout << "\n2. Ensuring resources are ready:\n";
    playerTexture.ensureReady();  // Loads to GPU
    jumpSound.ensureReady();      // Loads to audio hardware
    backgroundMusic.ensureReady(); // Loads music stream

    resourceManager.printResourceStats();

    std::cout << "\n3. Using resources:\n";
    if (playerTexture.isReady()) {
        std::cout << "Player texture ready for rendering\n";
        // playerTexture->draw(100, 100);
    }

    if (jumpSound.isReady()) {
        std::cout << "Jump sound ready for playback\n";
        // jumpSound->play();
    }

    std::cout << "\n4. Simulating GPU context loss (affects only textures):\n";
    resourceManager.handleGpuContextLost();
    std::cout << "Texture ready after context loss: " << (playerTexture.isReady() ? "Yes" : "No") << "\n";
    std::cout << "Sound ready after context loss: " << (jumpSound.isReady() ? "Yes" : "No") << "\n";

    std::cout << "\n5. Restoring GPU resources:\n";
    resourceManager.reloadAllGpuResources();
    playerTexture.ensureReady(); // Re-ensure GPU loading

    resourceManager.printResourceStats();

    std::cout << "\n6. Final cleanup...\n";

    return 0;
}
*/
