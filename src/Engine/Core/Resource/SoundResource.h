#pragma once
#include <string>
#include <format>
#include "raylib.h"

#include "Engine/Core/Resource/Resource.h"
#include "Engine/Core/Resource/ResourcePool.h"

namespace Struktur
{
	namespace Core
	{
		namespace Resource
		{
			// Raylib sound - Non-GPU resource (uses audio hardware)
			class SoundResource : public CpuResource
            {
			private:
				Wave m_waveData;
				
			public:
				Sound sound;
				
				SoundResource(const std::string& filePath) 
					: CpuResource(filePath)
                {
					sound.frameCount = 0;
					m_waveData.frameCount = 0;
				}
				
				~SoundResource()
                {
					UnloadFromHardware();
					UnloadFromDisk();
				}
				
				bool LoadFromDisk() override
                {
					if (isLoaded) return true;
					
					m_waveData = ::LoadWave(filePath.c_str());
					if (m_waveData.frameCount == 0) return false;
					
					isLoaded = true;
                    DEBUG_INFO(std::format("Loaded sound from disk: {}", filePath).c_str());
					return true;
				}
				
				void UnloadFromDisk() override
                {
					if (m_waveData.frameCount > 0) {
						::UnloadWave(m_waveData);
						m_waveData.frameCount = 0;
					}
					isLoaded = false;
				}
				
				bool LoadToHardware() override
                {
					if (!LoadFromDisk()) return false;
					if (sound.frameCount > 0) return true; // Already loaded
					
					sound = ::LoadSoundFromWave(m_waveData);
                    DEBUG_INFO(std::format("Loaded sound to audio hardware: {}", filePath).c_str());
					return sound.frameCount > 0;
				}
				
				void UnloadFromHardware() override
                {
					if (sound.frameCount > 0)
                    {
						::UnloadSound(sound);
						sound.frameCount = 0;
                        DEBUG_INFO(std::format("Unloaded sound from audio hardware: {}", filePath).c_str());
					}
				}
				
				bool IsHardwareReady() const override
                {
					return sound.frameCount > 0/* && IsSoundReady(sound)*/;
				}
				
				size_t GetMemoryUsage() const override
                {
					return isLoaded ? (m_waveData.frameCount * m_waveData.channels * (m_waveData.sampleSize / 8)) : 0;
				}
			};

			// Raylib music - Non-GPU resource (streaming audio)
			class MusicResource : public CpuResource
            {
			public:
				Music music;
				
				MusicResource(const std::string& filePath) 
					: CpuResource(filePath)
                {
					music.frameCount = 0;
				}
				
				~MusicResource()
                {
					UnloadFromHardware();
					UnloadFromDisk();
				}
				
				bool LoadFromDisk() override
                {
					if (isLoaded) return true;
					
					music = LoadMusicStream(filePath.c_str());
					if (music.frameCount == 0) return false;
					
					isLoaded = true;
                    DEBUG_INFO(std::format("Loaded music stream: {}", m_filePath).c_str());
					return true;
				}
				
				void UnloadFromDisk() override
                {
					if (music.frameCount > 0)
                    {
						UnloadMusicStream(music);
						music.frameCount = 0;
					}
					isLoaded = false;
				}
				
				bool LoadToHardware() override
                {
					return LoadFromDisk(); // Music streams don't need separate hardware loading
				}
				
				bool IsHardwareReady() const override
                {
					return music.frameCount > 0/* && IsMusicReady(music)*/;
				}
				
				size_t GetMemoryUsage() const override
                {
					return isLoaded ? 1024 * 1024 : 0; // Rough estimate for streaming buffer
				}
			};

			class SoundPool : public ResourcePool<SoundResource>
            {
			protected:
				SoundResource* LoadResource(const std::string& filePath) override
                {
					auto* sound = new SoundResource(filePath);
					
					if (!sound->LoadFromDisk())
                    {
						delete sound;
						return nullptr;
					}
					
					return sound;
				}
				
				void UnloadResource(const std::string& filePath, SoundResource* sound) override
                {
					delete sound;
				}
				
			public:
				bool EnsureResourceReady(const std::string& filePath) override
                {
					auto it = m_loadedResources.find(filePath);
					if (it == m_loadedResources.end()) return false;
					
					SoundResource* sound = it->second.resource;
					
					// Load from disk first
					if (!sound->isLoaded && !sound->LoadFromDisk())
                    {
						return false;
					}
					
					// Then load to audio hardware
					return sound->LoadToHardware();
				}
			};

			class MusicPool : public ResourcePool<MusicResource>
            {
			protected:
				MusicResource* LoadResource(const std::string& filePath) override {
					auto* music = new MusicResource(filePath);
					
					if (!music->LoadFromDisk()) {
						delete music;
						return nullptr;
					}
					
					return music;
				}
				
				void UnloadResource(const std::string& filePath, MusicResource* music) override {
					delete music;
				}
			};
        }
    }
}
