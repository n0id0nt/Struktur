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
		// Raylib texture - GPU resource
		class TextureResource : public GpuResource
		{
		private:
			::Image m_sourceImage;
			
		public:
			::Texture2D texture;
			
			TextureResource(const std::string& filePath);
			
			~TextureResource();
			
			bool LoadFromDisk() override;
			void UnloadFromDisk() override;
			bool LoadToGpu() override;
			void UnloadFromGpu() override;
			bool IsGpuResourceValid() const override;
			size_t GetMemoryUsage() const override;
			size_t GetGpuMemoryUsage() const override;
			int GetWidth() const { return m_sourceImage.width; }
			int GetHeight() const { return m_sourceImage.height; }
		};

		// Specialized pools
		class TexturePool : public GpuResourcePool<TextureResource>
		{
		public:
			TexturePool() : GpuResourcePool<TextureResource>(256 * 1024 * 1024) {} // 256MB for textures
			
		protected:
			TextureResource* LoadResource(const std::string& filePath) override;				
		};
	}
}
