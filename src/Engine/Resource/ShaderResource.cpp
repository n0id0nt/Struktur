#include "ShaderResource.h"

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"

#if !defined(PLATFORM_WEB)
	#include <algorithm>

	#include "Engine/Renderer/EmbeddedShaders.h"
#endif

Struktur::Resource::ShaderResource::ShaderResource(const std::string& vsFilePath, const std::string& fsFilePath)
    : GpuResource(vsFilePath + "," + fsFilePath),
      m_vsFilePath(vsFilePath),
      m_fsFilePath(fsFilePath)
{
#if defined(PLATFORM_WEB)
	shader.id   = 0;
	shader.locs = nullptr;
#else
	// Only one custom shader exists today (SoulEffect); anything else falls back to the default sprite shader.
	std::string lowerFsPath = m_fsFilePath;
	std::transform(lowerFsPath.begin(), lowerFsPath.end(), lowerFsPath.begin(), ::tolower);
	m_embeddedName = (lowerFsPath.find("souleffect") != std::string::npos) ? "soulEffect" : "sprite";
#endif
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

#if defined(PLATFORM_WEB)
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

	auto loadShaderSource = [](const std::string& path) -> std::string
	{
		if (path.empty())
		{
			return {};
		}

		auto result = FileSystem::ReadString(path);
		ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());
		return result.value;
	};

	std::string vsSource = loadShaderSource(m_vsFilePath);
	std::string fsSource = loadShaderSource(m_fsFilePath);

	const char* vs = vsSource.empty() ? nullptr : vsSource.c_str();
	const char* fs = fsSource.empty() ? nullptr : fsSource.c_str();

	shader = ::LoadShaderFromMemory(vs, fs);
	return shader.id != 0;
}

void Struktur::Resource::ShaderResource::UnloadFromGpu()
{
	if (shader.id != 0)
	{
		::UnloadShader(shader);
		shader.id   = 0;
		shader.locs = nullptr;
	}
}

bool Struktur::Resource::ShaderResource::IsGpuResourceValid() const
{
	return shader.id != 0;
}
#else
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
#endif

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

Struktur::Resource::ShaderResource* Struktur::Resource::ShaderPool::LoadResource(GameContext& context,
                                                                                 const std::string& resourceString)
{
	std::string vsFilePath;
	std::string fsFilePath;
	size_t commerPos = resourceString.find_last_of(',');
	if (commerPos != std::string::npos)
	{
		vsFilePath = resourceString.substr(0, commerPos);
		fsFilePath = resourceString.substr(commerPos + 1);
	}

	auto* shader = new ShaderResource(vsFilePath, fsFilePath);

	if (!shader->LoadFromDisk(context))
	{
		delete shader;
		return nullptr;
	}

	AddGpuResource(shader);
	return shader;
}
