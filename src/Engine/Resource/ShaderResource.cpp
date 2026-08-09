#include "ShaderResource.h"

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"

Struktur::Resource::ShaderResource::ShaderResource(const std::string& vsFilePath, const std::string& fsFilePath)
    : GpuResource(vsFilePath + "," + fsFilePath),
      m_vsFilePath(vsFilePath),
      m_fsFilePath(fsFilePath)
{
	shader.id   = 0;
	shader.locs = nullptr;
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
	// TODO store the entire shader file into a string
	DEBUG_INFO("Loaded shader from disk: vs = %s, fs = %s", m_vsFilePath.c_str(), m_fsFilePath.c_str());
	return true;
}

void Struktur::Resource::ShaderResource::UnloadFromDisk()
{
	// TODO once the shader loads to CPU clear it here
	isLoaded = false;
}

bool Struktur::Resource::ShaderResource::LoadToGpu()
{
	if (!isLoaded)
	{
		return false;
	}
	if (IsGpuResourceValid())
	{
		return true;
	}

	// Helper lambda to load shader source via PhysicsFS
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

size_t Struktur::Resource::ShaderResource::GetMemoryUsage() const
{
	if (!isLoaded)
	{
		return 0;
	}

	// Estimate: glyph data + texture data
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
