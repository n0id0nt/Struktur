#include "RiderAnimationSystem.h"

#include "Engine/ECS/Component/WorldComponents.h"
#include "Engine/GameContext.h"

Struktur::System::SystemAccess Struktur::System::RiderAnimationSystem::Access() const
{
	return SystemAccess::Worker()
	    .Read<World::World, Core::TimeSystem, Input::Input, Component::RoadPosition, Component::RiderSprite,
	          Component::Pacer>()
	    .Write<Component::CrankAnimation>();
}

void Struktur::System::RiderAnimationSystem::Prepare(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();
	registry.storage<Component::RoadPosition>();
	registry.storage<Component::CrankAnimation>();
	registry.storage<Component::RiderSprite>();
	registry.storage<Component::Pacer>();
}

void Struktur::System::RiderAnimationSystem::Update(GameContext& context)
{
	const World::World& world = context.GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	entt::registry& registry           = context.GetRegistry();
	const double delta                 = context.GetTimeSystem().unscaledDelta;
	const Input::TrainerInput& trainer = context.GetInput().GetTrainer();
	auto view                          = registry.view<Component::CrankAnimation, Component::RoadPosition>();
	for (auto [entity, crank, position] : view.each())
	{
		const double curvature = world.GetRoute().SampleAt(position.distance).curvature;
		if (registry.all_of<Component::RiderSprite>(entity))
		{
			crank.animation.Update(delta, trainer.cadenceRpm, trainer.speedMetersPerSecond, trainer.powerFraction,
			                       curvature);
		}
		else if (const Component::Pacer* pacer = registry.try_get<Component::Pacer>(entity))
		{
			crank.animation.Update(delta, pacer->cadenceRpm, pacer->speedMetersPerSecond, std::nullopt, curvature);
		}
	}
}
