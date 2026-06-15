#pragma once

#include "Engine/ECS/SystemManager.h"
#include "box2d/box2d.h"
#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace Component
{
struct PhysicsBody;
}
namespace System
{
class PhysicsSystem : public ISystem
{
   public:
	void Update(GameContext& context) override;

	void StepPhysics(GameContext& context, float deltaTime);
	void SyncPhysicsToTransforms(GameContext& context);
	void SyncTransformsToPhysics(GameContext& context);

	Component::PhysicsBody& CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef,
	                                          const b2Shape& shape);
	Component::PhysicsBody& CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef);

	std::string Name() const override
	{
		return "Physics System";
	}
};
}  // namespace System
}  // namespace Struktur
