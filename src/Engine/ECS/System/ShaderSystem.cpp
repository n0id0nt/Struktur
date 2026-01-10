#include "ShaderSystem.h"

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Shader.h"

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name, float value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);
	shader.floatUniforms[name] = value;
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name, int value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);
	shader.intUniforms[name] = value;
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::vec2 value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);
	shader.vec2Uniforms[name] = { value.x, value.y };
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::vec3 value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);
	shader.vec3Uniforms[name] = { value.x, value.y, value.z };
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::vec4 value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);
	shader.vec4Uniforms[name] = { value.x, value.y, value.z, value.w };
}

void Struktur::System::ShaderSystem::SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::mat4 value)
{
	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);

	::Matrix matrixUniform;
	matrixUniform.m0 = value[0][0];  matrixUniform.m4 = value[1][0];  matrixUniform.m8 = value[2][0];  matrixUniform.m12 = value[3][0];
	matrixUniform.m1 = value[0][1];  matrixUniform.m5 = value[1][1];  matrixUniform.m9 = value[2][1];  matrixUniform.m13 = value[3][1];
	matrixUniform.m2 = value[0][2];  matrixUniform.m6 = value[1][2];  matrixUniform.m10 = value[2][2];  matrixUniform.m14 = value[3][2];
	matrixUniform.m3 = value[0][3];  matrixUniform.m7 = value[1][3];  matrixUniform.m11 = value[2][3];  matrixUniform.m15 = value[3][3];

	shader.matrixUniforms[name] = matrixUniform;
}

void Struktur::System::ShaderSystem::ApplyUniforms(GameContext& context, entt::entity entity)
{
	Core::GameData& gameDate = context.GetGameData();
	SetUniform(context, entity, "time", (float)gameDate.gameTime);
	SetUniform(context, entity, "resolution", glm::vec2{ (float)gameDate.gameWidth, (float)gameDate.gameHeight });

	entt::registry& registry = context.GetRegistry();
	auto& shader = registry.get<Component::Shader>(entity);
	// Apply float uniforms
	for (const auto& [name, value] : shader.floatUniforms)
	{
		int location = GetCachedLocation(shader, name);
		::SetShaderValue(shader.shader->shader, location, &value, SHADER_UNIFORM_FLOAT);
	}

	// Apply int uniforms
	for (const auto& [name, value] : shader.intUniforms)
	{
		int location = GetCachedLocation(shader, name);
		::SetShaderValue(shader.shader->shader, location, &value, SHADER_UNIFORM_INT);
	}

	// Apply Vector2 uniforms
	for (const auto& [name, value] : shader.vec2Uniforms)
	{
		int location = GetCachedLocation(shader, name);
		::SetShaderValue(shader.shader->shader, location, &value, SHADER_UNIFORM_VEC2);
	}

	// Apply Vector3 uniforms
	for (const auto& [name, value] : shader.vec3Uniforms)
	{
		int location = GetCachedLocation(shader, name);
		::SetShaderValue(shader.shader->shader, location, &value, SHADER_UNIFORM_VEC3);
	}

	// Apply Vector4 uniforms
	for (const auto& [name, value] : shader.vec4Uniforms)
	{
		int location = GetCachedLocation(shader, name);
		::SetShaderValue(shader.shader->shader, location, &value, SHADER_UNIFORM_VEC4);
	}

	// Apply Matrix uniforms
	for (const auto& [name, value] : shader.matrixUniforms)
	{
		int location = GetCachedLocation(shader, name);
		::SetShaderValueMatrix(shader.shader->shader, location, value);
	}
}

void Struktur::System::ShaderSystem::BeginShader(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	auto* shader = registry.try_get<Component::Shader>(entity);
	if (!shader) return;
	if (!shader->shader->IsGpuReady())
	{
		if (!shader->shader->LoadToGpu())
		{
			BREAK_MSG("[SHADER] Error loading shader %s, %s", shader->shader->GetVSFilePath(), shader->shader->GetFSFilePath());
			return;
		}
	}
	ApplyUniforms(context, entity);
	::BeginShaderMode(shader->shader->shader);
}

void Struktur::System::ShaderSystem::EndShader(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	auto* shader = registry.try_get<Component::Shader>(entity);
	if (!shader) return;
	if (!shader->shader->IsGpuResourceValid()) return;
	::EndShaderMode();
}

int Struktur::System::ShaderSystem::GetCachedLocation(Component::Shader& shader, const std::string& name)
{
	if (shader.locationCache.find(name) == shader.locationCache.end())
	{
		shader.locationCache[name] = ::GetShaderLocation(shader.shader->shader, name.c_str());
	}
	return shader.locationCache[name];
}
