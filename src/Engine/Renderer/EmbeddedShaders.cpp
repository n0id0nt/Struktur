#include "EmbeddedShaders.h"

#include <cstring>
#include <string>
#include <unordered_map>

#include "Debug/Assertions.h"

// embedded_shader.h auto-detects WGSL support from the platform alone (BX_PLATFORM_WINDOWS etc, regardless of
// BGFX_CONFIG_RENDERER_WEBGPU), which would require _wgsl-suffixed symbols this project's shaders were never
// compiled for (CMakeLists.txt's struktur_compile_shader doesn't request the wgsl profile) - disabled explicitly.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>
#include <essl/fs_soulEffect.sc.bin.h>
#include <essl/fs_sprite.sc.bin.h>
#include <essl/vs_soulEffect.sc.bin.h>
#include <essl/vs_sprite.sc.bin.h>
#include <glsl/fs_soulEffect.sc.bin.h>
#include <glsl/fs_sprite.sc.bin.h>
#include <glsl/vs_soulEffect.sc.bin.h>
#include <glsl/vs_sprite.sc.bin.h>
#include <spirv/fs_soulEffect.sc.bin.h>
#include <spirv/fs_sprite.sc.bin.h>
#include <spirv/vs_soulEffect.sc.bin.h>
#include <spirv/vs_sprite.sc.bin.h>
#if defined(_WIN32)
	#include <dxbc/vs_sprite.sc.bin.h>
	#include <dxbc/fs_sprite.sc.bin.h>
	#include <dxbc/vs_soulEffect.sc.bin.h>
	#include <dxbc/fs_soulEffect.sc.bin.h>
	#include <dxil/vs_sprite.sc.bin.h>
	#include <dxil/fs_sprite.sc.bin.h>
	#include <dxil/vs_soulEffect.sc.bin.h>
	#include <dxil/fs_soulEffect.sc.bin.h>
#endif
#if defined(__APPLE__)
	#include <metal/vs_sprite.sc.bin.h>
	#include <metal/fs_sprite.sc.bin.h>
	#include <metal/vs_soulEffect.sc.bin.h>
	#include <metal/fs_soulEffect.sc.bin.h>
#endif

namespace
{
static const bgfx::EmbeddedShader kEmbeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_sprite),     BGFX_EMBEDDED_SHADER(fs_sprite), BGFX_EMBEDDED_SHADER(vs_soulEffect),
    BGFX_EMBEDDED_SHADER(fs_soulEffect), BGFX_EMBEDDED_SHADER_END(),
};

std::unordered_map<std::string, bgfx::ShaderHandle> g_shaderCache;
std::unordered_map<std::string, bgfx::ProgramHandle> g_programCache;
std::unordered_map<std::string, bgfx::UniformHandle> g_uniformCache;
}  // namespace

bgfx::ShaderHandle Struktur::Renderer::GetEmbeddedShader(const char* name)
{
	auto it = g_shaderCache.find(name);
	if (it != g_shaderCache.end())
	{
		return it->second;
	}

	bgfx::ShaderHandle handle = bgfx::createEmbeddedShader(kEmbeddedShaders, bgfx::getRendererType(), name);
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
