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
		// Raylib shader - GPU resource (contains texture atlas)
		class ShaderResource : public GpuResource
		{
		private:
			std::string m_vsFilePath;
			std::string m_fsFilePath;

		public:
			::Shader shader;

			ShaderResource(const std::string& vsFilePath, const std::string& fsFilePath);
			~ShaderResource();

			bool LoadFromDisk() override;
			void UnloadFromDisk() override;
			bool LoadToGpu() override;
			void UnloadFromGpu() override;

			bool IsGpuResourceValid() const override;

			size_t GetMemoryUsage() const override;
			size_t GetGpuMemoryUsage() const override;

			const std::string& GetVSFilePath() const;
			const std::string& GetFSFilePath() const;
		};

		// Specialized pools
		class ShaderPool : public GpuResourcePool<ShaderResource>
		{
		public:
			ShaderPool() : GpuResourcePool<ShaderResource>(1 * 1024 * 1024) {} // 1MB for textures

		protected:
			ShaderResource* LoadResource(const std::string& resourceString) override;
		};
	}
}
