#include "CameraSystem.h"

#include "Engine/ECS/Component/WorldComponents.h"
#include "Engine/GameContext.h"
#include "Engine/World/RoadFollow.h"

void Struktur::System::CameraSystem::Update(GameContext& context)
{
	const World::World& world = context.GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	entt::registry& registry = context.GetRegistry();
	double crankPhase        = 0.0;
	bool pedalling           = false;
	auto riders              = registry.view<Component::RiderSprite, Component::CrankAnimation>();
	for (auto [entity, crank] : riders.each())
	{
		crankPhase = crank.animation.GetCrankPhase();
		pedalling  = crank.animation.GetPose() != World::RiderPose::IDLE;
	}
	const double speed = context.GetInput().GetTrainer().speedMetersPerSecond;
	auto cameras       = registry.view<Component::Camera3D, World::RoadFollow>();
	for (auto [entity, camera, follow] : cameras.each())
	{
		follow.ApplySettings(world.GetSettings());
		World::FollowRoad(follow, world, context.GetTimeSystem().unscaledDelta, speed, crankPhase, pedalling,
		                  camera.camera);
		context.GetCamera() = camera.camera;
	}
}
