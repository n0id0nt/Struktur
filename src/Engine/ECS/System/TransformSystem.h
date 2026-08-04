#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
class GameContext;

namespace Component
{
struct Transform;
}

namespace System
{
class TransformSystem : public ISystem
{
   public:
	void Update(GameContext& context) override {}

	glm::vec3 WorldToLocal(GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity);

	void SetLocalTransform(GameContext& context, entt::entity entity, const glm::mat4& matrix);
	void SetLocalTransform(GameContext& context, entt::entity entity, const glm::vec3& position, const glm::vec3& scale,
	                       const glm::quat& rotation);
	void SetWorldTransform(GameContext& context, entt::entity entity, const glm::mat4& matrix);
	void SetWorldTransform(GameContext& context, entt::entity entity, const glm::vec3& position, const glm::vec3& scale,
	                       const glm::quat& rotation);
	// Writes local (and, for unparented entities, world) directly from an already-world-space
	// position/scale/rotation, skipping the matrix decompose used by SetWorldTransform. Falls back
	// to SetWorldTransform for entities with a parent, since those still need the parent-inverse to
	// keep local correct.
	void SetWorldTransformDirect(GameContext& context, entt::entity entity, const glm::vec3& position,
	                             const glm::vec3& scale, const glm::quat& rotation);

	// Pull getters: resolve the world-space cache if it's stale (this entity's local changed, or
	// an ancestor's did) before returning it. Code outside TransformSystem should always go through
	// these rather than reading Component::Transform's world* fields directly, since those may be stale.
	glm::vec3 GetWorldPosition(GameContext& context, entt::entity entity);
	glm::quat GetWorldRotation(GameContext& context, entt::entity entity);
	glm::vec3 GetWorldScale(GameContext& context, entt::entity entity);
	glm::mat4 GetWorldMatrix(GameContext& context, entt::entity entity);

	std::string Name() const override
	{
		return "Transform System";
	}

   private:
	// Resolves this entity's world-space cache if dirty (recursing on its parent first) and returns
	// the now-up-to-date component.
	Component::Transform& EnsureWorldUpToDate(GameContext& context, entt::entity entity);
	// Marks every descendant of entity dirty. Cheap (bool-only) - the actual recompute happens
	// lazily via EnsureWorldUpToDate the next time something reads a descendant's world transform.
	void CascadeDirty(entt::registry& registry, entt::entity entity);
};
}  // namespace System
}  // namespace Struktur
