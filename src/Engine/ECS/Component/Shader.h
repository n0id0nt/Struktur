#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

#include "Engine/Resource/ShaderResource.h"

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
	std::unordered_map<std::string, glm::vec2> vec2Uniforms;
	std::unordered_map<std::string, glm::vec3> vec3Uniforms;
	std::unordered_map<std::string, glm::vec4> vec4Uniforms;
	std::unordered_map<std::string, glm::mat4> matrixUniforms;
};
}  // namespace Component
}  // namespace Struktur
