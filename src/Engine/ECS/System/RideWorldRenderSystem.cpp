#include "RideWorldRenderSystem.h"

#include "Engine/GameContext.h"
#include "Engine/RideIntegration/RideService.h"

void Struktur::System::RideWorldRenderSystem::Update(GameContext& context)
{
	context.GetRideService().RenderWorld(context);
}
