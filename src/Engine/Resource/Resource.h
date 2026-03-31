#pragma once

#include <string>
#include <vector>
#include "physfs.h"

#include "Debug/Assertions.h"

namespace Struktur
{
	namespace Resource
	{
		// Base resource class
		class GameResource 
		{
		public:
			std::string filePath;
			bool isLoaded;
			
			GameResource(const std::string& filePath) 
				: filePath(filePath), isLoaded(false) {}
			virtual ~GameResource() = default;
			
			// Common resource management
			virtual bool LoadFromDisk() = 0;
			virtual void UnloadFromDisk() = 0;
			virtual size_t GetMemoryUsage() const = 0;

			static std::vector<uint8_t> LoadFile(const std::string& filePath)
			{
				PHYSFS_File* file = PHYSFS_openRead(filePath.c_str());
				ASSERT(file);

				PHYSFS_sint64 size = PHYSFS_fileLength(file);
				std::vector<uint8_t> buffer(size);
				PHYSFS_readBytes(file, buffer.data(), size);
				PHYSFS_close(file);

				return buffer;
			}
		};

		// GPU resource base class
		class GpuResource : public GameResource 
		{
		public:
			enum class GpuState 
			{
				Unloaded,      // Not loaded to GPU
				LoadedToGpu,   // Loaded to GPU and ready
				GpuLost        // GPU resource lost, need to reload
			};
			
			GpuState gpuState;
			
			GpuResource(const std::string& filePath) 
				: GameResource(filePath), gpuState(GpuState::Unloaded) {}
			
			// GPU-specific methods
			virtual bool LoadToGpu() = 0;
			virtual void UnloadFromGpu() = 0;
			virtual bool IsGpuResourceValid() const = 0;
			virtual size_t GetGpuMemoryUsage() const = 0;
			
			// State queries
			bool IsGpuReady() const { return gpuState == GpuState::LoadedToGpu && IsGpuResourceValid(); }
			bool NeedsGpuReload() const { return gpuState == GpuState::GpuLost || (isLoaded && gpuState == GpuState::Unloaded); }
		};

		// CPU resource base class (for sounds, music, config files, etc.)
		class CpuResource : public GameResource 
		{
		public:
			CpuResource(const std::string& filePath) 
				: GameResource(filePath) {}
			
			// CPU resources might have hardware-specific loading (audio devices, etc.)
			virtual bool LoadToHardware() { return true; } // Default: no special hardware loading
			virtual void UnloadFromHardware() {} // Default: no special hardware unloading
			virtual bool IsHardwareReady() const { return isLoaded; }
		};		
	}
}
