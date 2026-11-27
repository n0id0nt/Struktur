#include "PacerSystem.h"

#include "Engine/ECS/Component/WorldComponents.h"
#include "Engine/GameContext.h"
#include "Engine/RideIntegration/RideContext.h"
#include "Engine/World/Pacer.h"
#include "Engine/World/World.h"
#include "entt/entt.hpp"

Struktur::System::SystemAccess Struktur::System::PacerSystem::Access() const
{
	return SystemAccess::Worker()
	    .Read<World::World, Core::TimeSystem>()
	    .Write<Component::Pacer, Component::RoadPosition>();
}

void Struktur::System::PacerSystem::Prepare(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();
	registry.storage<Component::Pacer>();
	registry.storage<Component::RoadPosition>();
}

void Struktur::System::PacerSystem::Update(GameContext& context)
{
	const World::World& world = Struktur::RideContext::From(context).GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	const double delta = context.GetTimeSystem().unscaledDelta;
	auto view          = context.GetRegistry().view<Component::Pacer, Component::RoadPosition>();
	for (auto [entity, pacer, position] : view.each())
	{
		World::MovePacer(pacer, position, world.GetDistance(), delta);
	}
}
