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
			// Raylib texture - GPU resource
			class TextureResource : public GpuResource
			{
			private:
				unsigned char* m_imageData;
				int m_width, m_height, m_channels;
				
			public:
				::Texture2D texture;
				
				TextureResource(const std::string& filePath) 
					: GpuResource(filePath), m_imageData(nullptr), m_width(0), m_height(0), m_channels(0)
					{
					texture.id = 0;
				}
				
				~TextureResource()
				{
					UnloadFromGpu();
					UnloadFromDisk();
				}
				
				bool LoadFromDisk() override
				{
					if (isLoaded) return true;
					
					::Image image = ::LoadImage(filePath.c_str());
					if (image.data == nullptr) return false;
					
					m_width = image.width;
					m_height = image.height;
					m_channels = 4;
					
					size_t dataSize = m_width * m_height * m_channels;
					m_imageData = new unsigned char[dataSize];
					::ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
					memcpy(m_imageData, image.data, dataSize);
					
					UnloadImage(image);
					isLoaded = true;
					DEBUG_INFO(std::format("Loaded texture from disk: {} ({}x{})", name, width, height).c_str());
					return true;
				}
				
				void UnloadFromDisk() override
				{
					if (m_imageData)
					{
						delete[] m_imageData;
						m_imageData = nullptr;
					}
					isLoaded = false;
				}
				
				bool LoadToGpu() override
				{
					if (!LoadFromDisk()) return false;
					if (IsGpuResourceValid()) return true;
					
					Image image = {
						m_imageData,
						m_width,
						m_height,
						1,
						PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
					};
					
					texture = LoadTextureFromImage(image);
					return texture.id != 0;
				}
				
				void UnloadFromGpu() override
				{
					if (texture.id != 0) {
						UnloadTexture(texture);
						texture.id = 0;
					}
				}
				
				bool IsGpuResourceValid() const override
				{
					return texture.id != 0/* && IsTextureReady(texture)*/;
				}
				
				size_t GetMemoryUsage() const override
				{
					return isLoaded ? (m_width * m_height * m_channels) : 0;
				}
				
				size_t GetGpuMemoryUsage() const override
				{
					return GetMemoryUsage(); // Same as disk for simple case
				}
			};

			// Specialized pools
			class TexturePool : public GpuResourcePool<TextureResource> {
			public:
				TexturePool() : GpuResourcePool<TextureResource>(256 * 1024 * 1024) {} // 256MB for textures
				
			protected:
				TextureResource* LoadResource(const std::string& filePath) override
				{
					auto* texture = new TextureResource(filePath);
					
					if (!texture->LoadFromDisk())
					{
						delete texture;
						return nullptr;
					}
					
					AddGpuResource(texture);
					return texture;
				}
				
				void UnloadResource(const std::string& name, TextureResource* texture) override
				{
					RemoveGpuResource(texture);
					delete texture;
				}
			};
		}
	}
}
