#include "ShaderSystem.h"

#include "Engine/ECS/Component/Shader.h"
#include "Engine/GameContext.h"

#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/EmbeddedShaders.h"
#endif

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
	entt::registry& registry = context.GetRegistry();
	auto& shader             = registry.get<Component::Shader>(entity);

	::Matrix matrixUniform;
	matrixUniform.m0  = value[0][0];
	matrixUniform.m4  = value[1][0];
	matrixUniform.m8  = value[2][0];
	matrixUniform.m12 = value[3][0];
	matrixUniform.m1  = value[0][1];
	matrixUniform.m5  = value[1][1];
	matrixUniform.m9  = value[2][1];
	matrixUniform.m13 = value[3][1];
	matrixUniform.m2  = value[0][2];
	matrixUniform.m6  = value[1][2];
	matrixUniform.m10 = value[2][2];
	matrixUniform.m14 = value[3][2];
	matrixUniform.m3  = value[0][3];
	matrixUniform.m7  = value[1][3];
	matrixUniform.m11 = value[2][3];
	matrixUniform.m15 = value[3][3];

	shader.matrixUniforms[name] = matrixUniform;
}

#if !defined(PLATFORM_WEB)
void Struktur::System::ShaderSystem::ApplyUniforms(GameContext& context, entt::entity entity)
{
	Core::GameData& gameDate     = context.GetGameData();
	Core::TimeSystem& timeSystem = context.GetTimeSystem();
	SetUniform(context, entity, "time", (float)timeSystem.scaledTime);
	SetUniform(context, entity, "rawTime", (float)timeSystem.unscaledTime);
	SetUniform(context, entity, "resolution", glm::vec2{(float)gameDate.gameWidth, (float)gameDate.gameHeight});

	entt::registry& registry = context.GetRegistry();
	auto& shader              = registry.get<Component::Shader>(entity);

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
		float packed[16] = {value.m0, value.m1, value.m2,  value.m3,  value.m4,  value.m5,  value.m6,  value.m7,
		                    value.m8, value.m9, value.m10, value.m11, value.m12, value.m13, value.m14, value.m15};
		bgfx::setUniform(Renderer::GetOrCreateUniform(name), packed, 1);
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
	ApplyUniforms(context, entity);
	return shader->shader->shader;
}
#else
void Struktur::System::ShaderSystem::ApplyUniforms(GameContext& context, entt::entity entity)
{
	Core::GameData& gameDate     = context.GetGameData();
	Core::TimeSystem& timeSystem = context.GetTimeSystem();
	SetUniform(context, entity, "time", (float)timeSystem.scaledTime);
	SetUniform(context, entity, "rawTime", (float)timeSystem.unscaledTime);
	SetUniform(context, entity, "resolution", glm::vec2{(float)gameDate.gameWidth, (float)gameDate.gameHeight});

	entt::registry& registry = context.GetRegistry();
	auto& shader              = registry.get<Component::Shader>(entity);
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
	auto* shader             = registry.try_get<Component::Shader>(entity);
	if (!shader)
	{
		return;
	}
	if (!shader->shader->IsGpuReady())
	{
		if (!shader->shader->LoadToGpu(context))
		{
			BREAK_MSG("[SHADER] Error loading shader %s, %s", shader->shader->GetVSFilePath(),
			          shader->shader->GetFSFilePath());
			return;
		}
	}
	ApplyUniforms(context, entity);
	::BeginShaderMode(shader->shader->shader);
}

void Struktur::System::ShaderSystem::EndShader(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	auto* shader             = registry.try_get<Component::Shader>(entity);
	if (!shader)
	{
		return;
	}
	if (!shader->shader->IsGpuResourceValid())
	{
		return;
	}
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
#endif
