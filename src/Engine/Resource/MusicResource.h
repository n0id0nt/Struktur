#pragma once
#include <string>
#include <format>
#include "raylib.h"

#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"

namespace Struktur
{
	namespace Resource
	{
		// Raylib music - CPU resource (streaming audio)
		class MusicResource : public CpuResource
		{
		public:
			Music music;
			
			MusicResource(const std::string& filePath);			
			~MusicResource();
			
			bool LoadFromDisk() override;			
			void UnloadFromDisk() override;
			bool LoadToHardware() override;

			bool IsHardwareReady() const override;

			size_t GetMemoryUsage() const override;
		};

		class MusicPool : public ResourcePool<MusicResource>
		{
		protected:
			MusicResource* LoadResource(const std::string& filePath) override;
		};
	}
}
