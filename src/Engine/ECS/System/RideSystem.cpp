#include "RideSystem.h"

#include "Engine/GameContext.h"
#include "Engine/RideIntegration/RideService.h"

static constexpr double FINISH_BANNER_AHEAD_METERS = 90.0;

void Struktur::System::RideSystem::Update(GameContext& context)
{
	RideIntegration::RideService& service = context.GetRideService();
	World::World& world                   = context.GetWorld();
	// SIM simulates the world's road under the rider (in ERG the controller holds watts and ignores it).
	if (Ride::RideSession* session = service.GetSession())
	{
		session->SetRoadGrade(world.IsLoaded() ? std::optional<double>(world.GetRoadGradePercent()) : std::nullopt);
	}
	service.Update(context, context.GetTimeSystem().unscaledTime);
	if (service.TakeWorkoutFinished() && world.IsLoaded())
	{
		world.PlaceBanner(FINISH_BANNER_AHEAD_METERS);
	}
}
