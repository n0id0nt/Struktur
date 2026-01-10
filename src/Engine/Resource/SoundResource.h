#pragma once
#include <string>
#include "raylib.h"

#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"

namespace Struktur
{
	namespace Resource
	{
		// Raylib sound - CPU resource (uses audio hardware)
		class SoundResource : public CpuResource
		{
		private:
			Wave m_waveData;
			
		public:
			Sound sound;
			
			SoundResource(const std::string& filePath) ;
			~SoundResource();
			
			bool LoadFromDisk() override;			
			void UnloadFromDisk() override;			
			bool LoadToHardware() override;			
			void UnloadFromHardware() override;
			
			bool IsHardwareReady() const override;			
			size_t GetMemoryUsage() const override;
		};

		class SoundPool : public ResourcePool<SoundResource>
		{
		protected:
			SoundResource* LoadResource(const std::string& filePath) override;
			
		public:
			bool EnsureResourceReady(const std::string& filePath) override;
		};
	}
}
