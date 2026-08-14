#include "ShaderResource.h"

#include <algorithm>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/Renderer/EmbeddedShaders.h"

Struktur::Resource::ShaderResource::ShaderResource(const std::string& vsFilePath, const std::string& fsFilePath)
    : GpuResource(vsFilePath + "," + fsFilePath),
      m_vsFilePath(vsFilePath),
      m_fsFilePath(fsFilePath)
{
	// Only one custom shader exists today (SoulEffect); anything else falls back to the default sprite shader.
	std::string lowerFsPath = m_fsFilePath;
	std::transform(lowerFsPath.begin(), lowerFsPath.end(), lowerFsPath.begin(), ::tolower);
	m_embeddedName = (lowerFsPath.find("souleffect") != std::string::npos) ? "soulEffect" : "sprite";
}

Struktur::Resource::ShaderResource::~ShaderResource()
{
	UnloadFromGpu();
	UnloadFromDisk();
}

bool Struktur::Resource::ShaderResource::LoadFromDisk(GameContext& context)
{
	if (isLoaded)
	{
		return true;
	}

	isLoaded = true;
	DEBUG_INFO("Loaded shader from disk: vs = %s, fs = %s", m_vsFilePath.c_str(), m_fsFilePath.c_str());
	return true;
}

void Struktur::Resource::ShaderResource::UnloadFromDisk()
{
	isLoaded = false;
}

bool Struktur::Resource::ShaderResource::LoadToGpu(GameContext& context)
{
	if (!isLoaded)
	{
		return false;
	}
	if (IsGpuResourceValid())
	{
		return true;
	}

	shader = Renderer::GetEmbeddedProgram(m_embeddedName.c_str());
	return bgfx::isValid(shader);
}

void Struktur::Resource::ShaderResource::UnloadFromGpu()
{
	// Embedded programs are process-lifetime singletons shared by every ShaderResource with the same name
	// (see Renderer::GetEmbeddedProgram's cache) - only clear this instance's reference, don't destroy it.
	shader = BGFX_INVALID_HANDLE;
}

bool Struktur::Resource::ShaderResource::IsGpuResourceValid() const
{
	return bgfx::isValid(shader);
}

size_t Struktur::Resource::ShaderResource::GetMemoryUsage() const
{
	if (!isLoaded)
	{
		return 0;
	}

	size_t vsSize = m_vsFilePath.size();
	size_t fsSize = m_fsFilePath.size();
	return vsSize + fsSize;
}

size_t Struktur::Resource::ShaderResource::GetGpuMemoryUsage() const
{
	return GetMemoryUsage();
}

const std::string& Struktur::Resource::ShaderResource::GetVSFilePath() const
{
	return m_vsFilePath;
}

const std::string& Struktur::Resource::ShaderResource::GetFSFilePath() const
{
	return m_fsFilePath;
}
