#include "WorldLifecycle.h"

#include "Engine/ECS/Component/WorldComponents.h"
#include "Engine/ECS/System/CameraSystem.h"
#include "Engine/GameContext.h"
#include "Engine/World/RoadFollow.h"

namespace Struktur::World
{
static entt::entity CreateMember(entt::registry& registry)
{
	const entt::entity entity = registry.create();
	registry.emplace<Component::WorldMember>(entity);
	return entity;
}

static void CreateEntities(GameContext& context, bool demoPacer)
{
	entt::registry& registry = context.GetRegistry();

	const entt::entity rider = CreateMember(registry);
	registry.emplace<Component::RoadPosition>(rider);
	registry.emplace<Component::CrankAnimation>(rider);
	registry.emplace<Component::RiderSprite>(rider);

	const entt::entity camera = CreateMember(registry);
	registry.emplace<Component::Camera3D>(camera);
	registry.emplace<RoadFollow>(camera);

	if (demoPacer)
	{
		const entt::entity pacer = CreateMember(registry);
		registry.emplace<Component::RoadPosition>(pacer, Component::RoadPosition{0.0, 1.4});
		registry.emplace<Component::CrankAnimation>(pacer);
		registry.emplace<Component::Pacer>(pacer);
		registry.emplace<Component::Tint>(pacer, Component::Tint{0xFF60A0FFu});  // ABGR: orange jersey
	}
}

static void DestroyEntities(entt::registry& registry)
{
	const auto members = registry.view<Component::WorldMember>();
	registry.destroy(members.begin(), members.end());
}

bool LoadWorld(GameContext& context, const WorldSettings& settings, bool demoPacer)
{
	UnloadWorld(context);
	World& world              = context.GetWorld();
	const WorldAssets& assets = world.GetAssets();
	if (!settings.enabled || !assets.biomes || assets.biomes->empty())
	{
		return false;
	}
	world.Load(settings, &context.GetJobSystem());
	context.GetWorldRenderer().Initialise(context, assets.manifest, assets.spriteDescriptor);
	CreateEntities(context, demoPacer);
	// So the camera is already behind the rider if this frame still draws.
	context.GetSystemManager().GetSystem<System::CameraSystem>().Update(context);
	return true;
}

void UnloadWorld(GameContext& context)
{
	DestroyEntities(context.GetRegistry());
	context.GetWorldRenderer().Shutdown();
	context.GetWorld().Unload();
	context.GetCamera() = Camera();
}

void PlaceFinishBanner(GameContext& context, double aheadMeters)
{
	const World& world = context.GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	entt::registry& registry  = context.GetRegistry();
	const entt::entity banner = CreateMember(registry);
	registry.emplace<Component::RoadPosition>(banner, Component::RoadPosition{world.GetDistance() + aheadMeters, 0.0});
	registry.emplace<Component::Banner>(banner);
}
}  // namespace Struktur::World
