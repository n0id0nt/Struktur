#pragma once

#include <string>
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
}
