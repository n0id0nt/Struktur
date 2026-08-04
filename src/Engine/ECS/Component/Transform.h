#pragma once

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

struct LocalTransform
{
	glm::mat4 matrix{1.0f};
	glm::vec3 position{0.0f};
	glm::vec3 scale{1.0f};
	glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct WorldTransform
{
	glm::mat4 matrix{1.0f};
	glm::vec3 position{0.0f};
	glm::vec3 scale{1.0f};
	glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
	// True when this transform has changed since physics last read it. Defaults to true so a
	// freshly created/emplaced transform always gets its first sync.
	bool dirty{true};
};
}  // namespace Component
}  // namespace Struktur
