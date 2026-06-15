#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

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

	void BeginShader(GameContext& context, entt::entity entity);
	void EndShader(GameContext& context, entt::entity entity);

	std::string Name() const override
	{
		return "Shader System";
	}

   private:
	int GetCachedLocation(Component::Shader& shader, const std::string& name);
};
}  // namespace System
}  // namespace Struktur
