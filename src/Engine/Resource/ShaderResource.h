#pragma once

#include <string>

#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"
#include "raylib.h"

namespace Struktur
{
class GameContext;
}

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

	bool LoadFromDisk(GameContext& context) override;
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
	ShaderPool()
	    : GpuResourcePool<ShaderResource>(1 * 1024 * 1024)
	{
	}  // 1MB for textures

   protected:
	ShaderResource* LoadResource(GameContext& context, const std::string& resourceString) override;
};
}  // namespace Resource
}  // namespace Struktur
