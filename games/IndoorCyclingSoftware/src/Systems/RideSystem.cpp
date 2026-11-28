#include "RideSystem.h"

#include "Engine/GameContext.h"
#include "RideContext.h"
#include "RideIntegration/RideService.h"
#include "World/World.h"
#include "World/WorldLifecycle.h"

static constexpr double FINISH_BANNER_AHEAD_METERS = 90.0;

void Struktur::System::RideSystem::Update(GameContext& context)
{
	RideIntegration::RideService& service = Struktur::RideContext::From(context).GetRideService();
	World::World& world                   = Struktur::RideContext::From(context).GetWorld();
	// SIM simulates the world's road under the rider (in ERG the controller holds watts and ignores it).
	if (Ride::RideSession* session = service.GetSession())
	{
		session->SetRoadGrade(world.IsLoaded() ? std::optional<double>(world.GetRoadGradePercent()) : std::nullopt);
	}
	service.Update(context, context.GetTimeSystem().unscaledTime);
	if (service.TakeWorkoutFinished())
	{
		World::PlaceFinishBanner(context, FINISH_BANNER_AHEAD_METERS);
	}
}
