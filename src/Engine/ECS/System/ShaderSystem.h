#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include <bgfx/bgfx.h>

namespace Struktur
{
class GameContext;

namespace Component
{
struct Shader;
}
namespace System
{
class ShaderSystem : public ISystem
{
   public:
	void Update(GameContext& context) override {}

	void SetUniform(GameContext& context, entt::entity entity, const std::string& name, float value);
	void SetUniform(GameContext& context, entt::entity entity, const std::string& name, int value);
	void SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::vec2 value);
	void SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::vec3 value);
	void SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::vec4 value);
	void SetUniform(GameContext& context, entt::entity entity, const std::string& name, glm::mat4 value);
	void ApplyUniforms(GameContext& context, entt::entity entity);

	// bgfx has no Begin/EndShaderMode-style block - the batch renderer needs to know the program up front to
	// detect same-shader runs, so it resolves a program per entity instead of wrapping each draw call.
	bgfx::ProgramHandle ResolveProgram(GameContext& context, entt::entity entity, bgfx::ProgramHandle defaultProgram);

	std::string Name() const override
	{
		return "Shader System";
	}
};
}  // namespace System
}  // namespace Struktur
