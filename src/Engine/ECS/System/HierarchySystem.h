#pragma once

#include "Engine/ECS/Component/Active.h"
#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
class GameContext;

namespace System
{
class HierarchySystem : public ISystem
{
   public:
	HierarchySystem() {}

	void Update(GameContext& context) override {}  // Empty Update loop

	void SetParent(GameContext& context, entt::entity child, entt::entity parent);
	void RemoveFromParent(GameContext& context, entt::entity child, entt::entity parent);
	void DestroyEntity(GameContext& context, entt::entity entity);
	void ActivevateEntity(GameContext& context, entt::entity entity);
	void DeactivevateEntity(GameContext& context, entt::entity entity);

	std::string Name() const override
	{
		return "Hierarchy System";
	}

   private:
	void PropagateActiveStateToChildren(GameContext& context, entt::entity entity,
	                                    Component::Active::ActiveState active);
};
}  // namespace System
}  // namespace Struktur
