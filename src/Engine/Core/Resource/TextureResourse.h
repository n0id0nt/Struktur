#pragma once
#include <string>
#include "raylib.h"

#include "Engine/Core/Resourse/Resourse.h"

namespace Struktur
{
	namespace Core
	{
        class ResourcePool;

		class TextureResourse : IResourse
		{
		public:
			TextureResourse(ResourcePool* resourcePool, const std::string& path) : IResourse(resoursePool, path) {}
			~TextureResourse();
		
            bool IsLoaded() override;
            
            bool IsTextureLoadedInGPU(const std::string& path) const;
			void LoadTextureInGPU(const std::string& path);
			Texture2D RetrieveTexture(const std::string& path) const;
			Image RetrieveImage(const std::string& path) const;
			void UnloadTextureGPU(const std::string& path);
			void ReleaseTexture(const std::string& path);

		private:
			ResourcePool* m_resoursePool;
			std::string m_path;
		};
	}
}
