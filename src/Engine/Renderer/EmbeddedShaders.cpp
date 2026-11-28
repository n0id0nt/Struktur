#include "EmbeddedShaders.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "Debug/Assertions.h"

// embedded_shader.h auto-detects WGSL support from the platform alone, which would require _wgsl-suffixed symbols this
// project's shaders are never compiled for (struktur_add_shaders doesn't request the wgsl profile) - disabled.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>

static std::vector<const bgfx::EmbeddedShader*> g_shaderTables;
static std::unordered_map<std::string, bgfx::ShaderHandle> g_shaderCache;
static std::unordered_map<std::string, bgfx::ProgramHandle> g_programCache;
static std::unordered_map<std::string, bgfx::UniformHandle> g_uniformCache;

// The registered table holding a shader of that name, or null.
static const bgfx::EmbeddedShader* FindTable(const char* name)
{
	for (const bgfx::EmbeddedShader* table : g_shaderTables)
	{
		for (const bgfx::EmbeddedShader* shader = table; shader->name != nullptr; ++shader)
		{
			if (std::strcmp(shader->name, name) == 0)
			{
				return table;
			}
		}
	}
	return nullptr;
}

void Struktur::Renderer::RegisterEmbeddedShaders(const bgfx::EmbeddedShader* shaders)
{
	if (std::find(g_shaderTables.begin(), g_shaderTables.end(), shaders) == g_shaderTables.end())
	{
		g_shaderTables.push_back(shaders);
	}
}

bgfx::ShaderHandle Struktur::Renderer::GetEmbeddedShader(const char* name)
{
	auto it = g_shaderCache.find(name);
	if (it != g_shaderCache.end())
	{
		return it->second;
	}

	const bgfx::EmbeddedShader* table = FindTable(name);
	ASSERT_MSG(table != nullptr, "No registered shader table has '%s'", name);
	bgfx::ShaderHandle handle = BGFX_INVALID_HANDLE;
	if (table != nullptr)
	{
		handle = bgfx::createEmbeddedShader(table, bgfx::getRendererType(), name);
	}
	ASSERT_MSG(bgfx::isValid(handle), "Failed to create embedded shader '%s'", name);
	g_shaderCache[name] = handle;
	return handle;
}

bgfx::ProgramHandle Struktur::Renderer::GetEmbeddedProgram(const char* name)
{
	auto it = g_programCache.find(name);
	if (it != g_programCache.end())
	{
		return it->second;
	}

	std::string vsName    = std::string("vs_") + name;
	std::string fsName    = std::string("fs_") + name;
	bgfx::ShaderHandle vs = GetEmbeddedShader(vsName.c_str());
	bgfx::ShaderHandle fs = GetEmbeddedShader(fsName.c_str());

	bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, false);
	ASSERT_MSG(bgfx::isValid(program), "Failed to link embedded program '%s'", name);
	g_programCache[name] = program;
	return program;
}

bgfx::UniformHandle Struktur::Renderer::GetOrCreateUniform(const std::string& name)
{
	auto it = g_uniformCache.find(name);
	if (it != g_uniformCache.end())
	{
		return it->second;
	}

	bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Vec4);
	g_uniformCache[name]       = handle;
	return handle;
}

void Struktur::Renderer::Shutdown()
{
	for (auto& [name, handle] : g_programCache)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	g_programCache.clear();

	for (auto& [name, handle] : g_shaderCache)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	g_shaderCache.clear();

	for (auto& [name, handle] : g_uniformCache)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	g_uniformCache.clear();
}
