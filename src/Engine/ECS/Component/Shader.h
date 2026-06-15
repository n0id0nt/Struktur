#pragma once

#include <unordered_map>

#include "Engine/Resource/ShaderResource.h"
#include "raylib.h"

namespace Struktur
{
namespace Component
{
struct Shader
{
	Resource::ResourcePtr<Resource::ShaderResource> shader;

	// Separate dictionaries for each uniform type
	std::unordered_map<std::string, float> floatUniforms;
	std::unordered_map<std::string, int> intUniforms;
	std::unordered_map<std::string, ::Vector2> vec2Uniforms;
	std::unordered_map<std::string, ::Vector3> vec3Uniforms;
	std::unordered_map<std::string, ::Vector4> vec4Uniforms;
	std::unordered_map<std::string, ::Matrix> matrixUniforms;

	std::unordered_map<std::string, int> locationCache;
};
}  // namespace Component
}  // namespace Struktur
