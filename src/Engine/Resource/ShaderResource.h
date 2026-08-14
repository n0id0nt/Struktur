#pragma once

#include <string>

#include <bgfx/bgfx.h>

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/ResourcePool.h"
#include "Engine/Resource/Resource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// GPU shader program - bgfx, precompiled/embedded (see Engine/Renderer/EmbeddedShaders).
class ShaderResource : public GpuResource
{
   private:
	std::string m_vsFilePath;
	std::string m_fsFilePath;
	// The embedded shader pair's name (e.g. "soulEffect") derived from m_fsFilePath - there's no runtime shader
	// text to read on this path, so the file paths only identify *which* compiled-in shader to use.
	std::string m_embeddedName;

   public:
	bgfx::ProgramHandle shader = BGFX_INVALID_HANDLE;

	ShaderResource(const std::string& vsFilePath, const std::string& fsFilePath);
	~ShaderResource();

	bool LoadFromDisk(GameContext& context) override;
	void UnloadFromDisk() override;
	bool LoadToGpu(GameContext& context) override;
	void UnloadFromGpu() override;

	bool IsGpuResourceValid() const override;

	size_t GetMemoryUsage() const override;
	size_t GetGpuMemoryUsage() const override;

	const std::string& GetVSFilePath() const;
	const std::string& GetFSFilePath() const;
};
}  // namespace Resource
}  // namespace Struktur
