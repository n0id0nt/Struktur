#include "ShaderSystem.h"

#include "Engine/ECS/Component/Shader.h"
#include "Engine/GameContext.h"

#include <glm/gtc/type_ptr.hpp>

#include "Engine/Renderer/EmbeddedShaders.h"

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name,
                                                float value)
{
	entt::registry& registry   = context.GetRegistry();
	auto& shader               = registry.get<Component::Shader>(entity);
	shader.floatUniforms[name] = value;
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name,
                                                int value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader             = registry.get<Component::Shader>(entity);
	shader.intUniforms[name] = value;
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name,
                                                glm::vec2 value)
{
	entt::registry& registry  = context.GetRegistry();
	auto& shader              = registry.get<Component::Shader>(entity);
	shader.vec2Uniforms[name] = {value.x, value.y};
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name,
                                                glm::vec3 value)
{
	entt::registry& registry  = context.GetRegistry();
	auto& shader              = registry.get<Component::Shader>(entity);
	shader.vec3Uniforms[name] = {value.x, value.y, value.z};
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name,
                                                glm::vec4 value)
{
	entt::registry& registry  = context.GetRegistry();
	auto& shader              = registry.get<Component::Shader>(entity);
	shader.vec4Uniforms[name] = {value.x, value.y, value.z, value.w};
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name,
                                                glm::mat4 value)
{
	entt::registry& registry    = context.GetRegistry();
	auto& shader                = registry.get<Component::Shader>(entity);
	shader.matrixUniforms[name] = value;
}

void Struktur::System::ShaderSystem::ApplyUniforms(GameContext& context, const Component::Shader& shader)
{
	Core::GameData& gameDate     = context.GetGameData();
	Core::TimeSystem& timeSystem = context.GetTimeSystem();
	float packedTime[4]          = {(float)timeSystem.scaledTime, 0.0f, 0.0f, 0.0f};
	float packedRawTime[4]       = {(float)timeSystem.unscaledTime, 0.0f, 0.0f, 0.0f};
	float packedResolution[4]    = {(float)gameDate.gameWidth, (float)gameDate.gameHeight, 0.0f, 0.0f};
	bgfx::setUniform(Renderer::GetOrCreateUniform("time"), packedTime);
	bgfx::setUniform(Renderer::GetOrCreateUniform("rawTime"), packedRawTime);
	bgfx::setUniform(Renderer::GetOrCreateUniform("resolution"), packedResolution);

	// Every bgfx uniform is a vec4 register - scalars/vec2/vec3 pad the unused components with 0 (see the
	// vs_soulEffect.sc/fs_soulEffect.sc uniform declarations, which read back only the components they need).
	for (const auto& [name, value] : shader.floatUniforms)
	{
		float packed[4] = {value, 0.0f, 0.0f, 0.0f};
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), packed);
	}
	for (const auto& [name, value] : shader.intUniforms)
	{
		float packed[4] = {(float)value, 0.0f, 0.0f, 0.0f};
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), packed);
	}
	for (const auto& [name, value] : shader.vec2Uniforms)
	{
		float packed[4] = {value.x, value.y, 0.0f, 0.0f};
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), packed);
	}
	for (const auto& [name, value] : shader.vec3Uniforms)
	{
		float packed[4] = {value.x, value.y, value.z, 0.0f};
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), packed);
	}
	for (const auto& [name, value] : shader.vec4Uniforms)
	{
		float packed[4] = {value.x, value.y, value.z, value.w};
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), packed);
	}
	for (const auto& [name, value] : shader.matrixUniforms)
	{
		// glm::mat4 is already column-major, matching what bgfx::setUniform expects for a mat4 - no repacking.
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), glm::value_ptr(value), 1);
	}
}

bgfx::ProgramHandle Struktur::System::ShaderSystem::ResolveProgram(GameContext& context, entt::entity entity,
                                                                    bgfx::ProgramHandle defaultProgram)
{
	entt::registry& registry = context.GetRegistry();
	auto* shader              = registry.try_get<Component::Shader>(entity);
	if (!shader)
	{
		return defaultProgram;
	}
	if (!shader->shader->IsGpuReady())
	{
		if (!shader->shader->LoadToGpu(context))
		{
			BREAK_MSG("[SHADER] Error loading shader %s, %s", shader->shader->GetVSFilePath(),
			          shader->shader->GetFSFilePath());
			return defaultProgram;
		}
	}
	// Uniforms are NOT applied here - see ApplyUniforms's own comment. This only resolves/loads the program, so
	// it's safe for callers (SpriteRenderSystem) to call it well ahead of the actual submit for batching/sort-key
	// purposes.
	return shader->shader->shader;
}
