#include "WorldSystem.h"

#include <vector>

#include "Components/WorldComponents.h"
#include "Engine/GameContext.h"
#include "RideContext.h"
#include "World/World.h"
#include "entt/entt.hpp"

static constexpr double BANNER_KEPT_BEHIND_METERS = 20.0;

void Struktur::System::WorldSystem::Update(GameContext& context)
{
	World::World& world = Struktur::RideContext::From(context).GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	world.Update(context.GetTimeSystem().unscaledDelta, context.GetInput().GetTrainer());

	entt::registry& registry = context.GetRegistry();
	auto riders              = registry.view<Component::RiderSprite, Component::RoadPosition>();
	for (auto [entity, position] : riders.each())
	{
		position.distance = world.GetDistance();
	}
	std::vector<entt::entity> passed;
	auto banners = registry.view<Component::Banner, Component::RoadPosition>();
	for (auto [entity, position] : banners.each())
	{
		if (position.distance < world.GetDistance() - BANNER_KEPT_BEHIND_METERS)
		{
			passed.push_back(entity);
		}
	}
	registry.destroy(passed.begin(), passed.end());
}
