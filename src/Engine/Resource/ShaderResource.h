#pragma once

#include <string>

#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourcePool.h"
#include "Engine/Resource/ResourcePtr.h"

#if defined(PLATFORM_WEB)
	#include "raylib.h"
#else
	#include <bgfx/bgfx.h>
#endif

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// GPU shader program - bgfx on desktop (precompiled/embedded, see Engine/Renderer/EmbeddedShaders), raylib on
// web (raw GLSL loaded from disk, see LoadToGpu/UnloadFromGpu).
class ShaderResource : public GpuResource
{
   private:
	std::string m_vsFilePath;
	std::string m_fsFilePath;
#if !defined(PLATFORM_WEB)
	// The embedded shader pair's name (e.g. "soulEffect") derived from m_fsFilePath - there's no runtime shader
	// text to read on this path, so the file paths only identify *which* compiled-in shader to use.
	std::string m_embeddedName;
#endif

   public:
#if defined(PLATFORM_WEB)
	::Shader shader;
#else
	bgfx::ProgramHandle shader = BGFX_INVALID_HANDLE;
#endif

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
