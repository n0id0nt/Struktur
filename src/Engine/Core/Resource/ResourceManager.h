#pragma once
#include <string>
#include <format>
#include "raylib.h"

#include "Engine/Core/Resource/Resource.h"
#include "Engine/Core/Resource/ResourcePtr.h"
#include "Engine/Core/Resource/ResourcePool.h"
#include "Engine/Core/Resource/SoundResource.h"
#include "Engine/Core/Resource/MusicResource.h"
#include "Engine/Core/Resource/TextureResource.h"
#include "Engine/Core/Resource/FontResource.h"

namespace Struktur
{
	namespace Core
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
				FontPool m_fontResource;
				
			public:
				ResourcePtr<TextureResource> GetTexture(const std::string& name)
                {
					return m_texturePool.GetResource(name);
				}
				
				ResourcePtr<SoundResource> GetSound(const std::string& name)
                {
					return m_soundPool.GetResource(name);
				}
				
				ResourcePtr<MusicResource> GetMusic(const std::string& name)
                {
					return m_musicPool.GetResource(name);
				}
				
				ResourcePtr<FontResource> GetFontResource(const std::string& name)
                {
					return m_fontResource.GetResource(name);
				}
				
				// GPU-specific operations (only affect GPU resources)
				void HandleGpuContextLost() {
                    DEBUG_INFO("=== GPU CONTEXT LOST ===");
					m_texturePool.HandleGpuContextLost();
					// Note: Sound and music pools are unaffected
				}
				
				void ReloadAllGpuResources() {
                    DEBUG_INFO("=== RELOADING GPU RESOURCES ===");
					m_texturePool.ReloadAllGpuResources();
					// Note: Sound and music pools are unaffected
				}
				
				void PrintResourceStats() const {
					DEBUG_INFO("=== Resource Statistics ===");
					// GPU resources
					//std::cout << "GPU Resources (Textures):\n";
					//std::cout << "  Count: " << m_texturePool.getLoadedCount() << "\n";
					//std::cout << "  GPU Memory: " << m_texturePool.getGpuMemoryUsage() / 1024 / 1024 
					//		<< "MB / " << m_texturePool.getMaxGpuMemory() / 1024 / 1024 
					//		<< "MB (" << m_texturePool.getGpuMemoryUsagePercent() << "%)\n";
					//std::cout << "  System Memory: " << m_texturePool.getTotalMemoryUsage() / 1024 / 1024 << "MB\n";
					//
					//// Non-GPU resources
					//std::cout << "Audio Resources:\n";
					//std::cout << "  Sounds: " << m_soundPool.getLoadedCount() 
					//		<< " (" << m_soundPool.getTotalMemoryUsage() / 1024 / 1024 << "MB)\n";
					//std::cout << "  Music: " << m_musicPool.getLoadedCount() 
					//		<< " (" << m_musicPool.getTotalMemoryUsage() / 1024 / 1024 << "MB)\n";
				}
			};
        }
    }
}
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
