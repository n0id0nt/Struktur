#pragma once
#include <string>
#include <format>
#include "raylib.h"

#include "Engine/Core/Resource/Resource.h"
#include "Engine/Core/Resource/ResourcePool.h"
#include "Engine/Core/Resource/ResourcePtr.h"

namespace Struktur
{
	namespace Core
	{
		namespace Resource
		{
			// Raylib music - CPU resource (streaming audio)
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
                    DEBUG_INFO(std::format("Loaded music stream: {}", filePath).c_str());
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

			class MusicPool : public ResourcePool<MusicResource>
            {
			protected:
				MusicResource* LoadResource(const std::string& filePath) override
				{
					auto* music = new MusicResource(filePath);
					
					if (!music->LoadFromDisk()) {
						delete music;
						return nullptr;
					}
					
					return music;
				}
			};
        }
    }
}
