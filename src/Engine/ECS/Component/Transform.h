#pragma once

#include <cstdint>

#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
namespace Component
{
struct Parent
{
	entt::entity entity = entt::null;
};

struct Children
{
	std::vector<entt::entity> entities;
};

// Local space is always authoritative and up to date. World space is a cache lazily recomputed
// by TransformSystem the first time something reads it after local (or an ancestor) changes -
// code outside TransformSystem should read world* through TransformSystem's getters rather than
// these fields directly, since a stale cache is only refreshed on that pull.
struct Transform
{
	glm::mat4 localMatrix{1.0f};
	glm::vec3 localPosition{0.0f};
	glm::vec3 localScale{1.0f};
	glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};

	glm::mat4 worldMatrix{1.0f};
	glm::vec3 worldPosition{0.0f};
	glm::vec3 worldScale{1.0f};
	glm::quat worldRotation{1.0f, 0.0f, 0.0f, 0.0f};
	// True when worldMatrix/worldPosition/worldScale/worldRotation no longer reflect local (or an
	// ancestor's world). Defaults to true so a freshly created/emplaced transform is resolved on
	// first read.
	bool dirty{true};

	// Bumped by TransformSystem::SetLocalTransform (a real change to this entity's own local matrix) and by
	// CascadeDirty (an ancestor changed, so this entity's effective world position did too) - never touched by
	// a read. Unlike `dirty`, which any reader can consume as a side effect of caching the world matrix, this
	// only ever increases, so a consumer (see Component::PhysicsBody::syncedTransformVersion) that remembers
	// the version it last handled can reliably detect "something changed since I last looked", independent of
	// whether anything else already read (and cleared) `dirty` first.
	uint32_t version{0};
};
}  // namespace Component
}  // namespace Struktur
