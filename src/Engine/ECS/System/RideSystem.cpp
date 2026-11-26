#include "RideSystem.h"

#include "Engine/GameContext.h"
#include "Engine/RideIntegration/RideService.h"

void Struktur::System::RideSystem::Update(GameContext& context)
{
	context.GetRideService().Update(context, context.GetTimeSystem().unscaledTime);
}
