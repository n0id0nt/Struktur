#include "ShaderResource.h"

Struktur::Resource::ShaderResource::ShaderResource(const std::string &vsFilePath, const std::string &fsFilePath)
    : GpuResource(vsFilePath + "," + fsFilePath), m_vsFilePath(vsFilePath), m_fsFilePath(fsFilePath)
{
    shader.id = 0;
    shader.locs = nullptr;
}

Struktur::Resource::ShaderResource::~ShaderResource()
{
    UnloadFromGpu();
    UnloadFromDisk();
}

bool Struktur::Resource::ShaderResource::LoadFromDisk()
{
    if (isLoaded) return true;

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
    if (!LoadFromDisk()) return false;
    if (IsGpuResourceValid()) return true;

    const char* vsFilePath = m_vsFilePath.empty() ? nullptr : m_vsFilePath.c_str();
    const char* fsFilePath = m_fsFilePath.empty() ? nullptr : m_fsFilePath.c_str();

    shader = ::LoadShader(vsFilePath, fsFilePath);
    return shader.id != 0;
}

void Struktur::Resource::ShaderResource::UnloadFromGpu()
{
    if (shader.id != 0)
    {
        ::UnloadShader(shader);
        shader.id = 0;
        shader.locs = nullptr;
    }
}

bool Struktur::Resource::ShaderResource::IsGpuResourceValid() const
{
    return shader.id != 0;
}

size_t Struktur::Resource::ShaderResource::GetMemoryUsage() const
{
    if (!isLoaded) return 0;

    // Estimate: glyph data + texture data
    size_t vsSize = m_vsFilePath.size();
    size_t fsSize = m_fsFilePath.size();
    return vsSize + fsSize;
}

size_t Struktur::Resource::ShaderResource::GetGpuMemoryUsage() const
{
    return GetMemoryUsage();
}

const std::string &Struktur::Resource::ShaderResource::GetVSFilePath() const
{
    return m_vsFilePath;
}

const std::string &Struktur::Resource::ShaderResource::GetFSFilePath() const
{
    return m_fsFilePath;
}

Struktur::Resource::ShaderResource *Struktur::Resource::ShaderPool::LoadResource(const std::string &resourceString)
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

    if (!shader->LoadFromDisk())
    {
        delete shader;
        return nullptr;
    }

    AddGpuResource(shader);
    return shader;
}
