#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
class GameContext;

namespace System
{
class TransformSystem : public ISystem
{
   public:
	void Update(GameContext& context) override {}

	glm::vec3 WorldToLocal(GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity);
	float GetWorldRotation(GameContext& context, entt::entity entity);
	void SetLocalTransform(GameContext& context, entt::entity entity, const glm::mat4& matrix);
	void SetLocalTransform(GameContext& context, entt::entity entity, const glm::vec3& position, const glm::vec3& scale,
	                       const glm::quat& rotation);
	void SetWorldTransform(GameContext& context, entt::entity entity, const glm::mat4& matrix);
	void SetWorldTransform(GameContext& context, entt::entity entity, const glm::vec3& position, const glm::vec3& scale,
	                       const glm::quat& rotation);
	// Writes WorldTransform (and, for unparented entities, LocalTransform) directly from an
	// already-world-space position/scale/rotation, skipping the matrix decompose used by
	// SetWorldTransform. Falls back to SetWorldTransform for entities with a parent, since those
	// still need the parent-inverse to keep LocalTransform correct.
	void SetWorldTransformDirect(GameContext& context, entt::entity entity, const glm::vec3& position,
	                             const glm::vec3& scale, const glm::quat& rotation);

	std::string Name() const override
	{
		return "Transform System";
	}

   private:
	void UpdateWorldTransform(GameContext& context, entt::entity entity, const glm::mat4& parentMatrix);
};
}  // namespace System
}  // namespace Struktur
