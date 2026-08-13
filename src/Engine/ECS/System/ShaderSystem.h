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

	// Takes the resolved Component::Shader directly rather than an entity - this must be called immediately
	// before the bgfx::submit() that actually uses these values (see WorldRenderer::Flush/FlushRun), not any
	// earlier, since bgfx::setUniform's state is only valid for the next submit on this encoder. Resolving via
	// entity+registry here would tempt calling this eagerly at submit-collection time again, which is exactly
	// what caused uniform values from an earlier entity to still be "pending" when a later entity's ApplyUniforms
	// ran before either had actually been submitted - bgfx flags that as re-setting an already-set uniform.
	void ApplyUniforms(GameContext& context, const Component::Shader& shader);

	// bgfx has no Begin/EndShaderMode-style block - the batch renderer needs to know the program up front to
	// detect same-shader runs, so it resolves a program per entity instead of wrapping each draw call. Safe to
	// call ahead of the actual submit (unlike ApplyUniforms above) - this only loads the shader's GPU resource
	// and returns its handle, it doesn't touch any per-draw bgfx state.
	bgfx::ProgramHandle ResolveProgram(GameContext& context, entt::entity entity, bgfx::ProgramHandle defaultProgram);

	std::string Name() const override
	{
		return "Shader System";
	}
};
}  // namespace System
}  // namespace Struktur
