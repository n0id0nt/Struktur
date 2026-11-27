#include "WorldRenderSystem.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/RideWorldRenderer.h"

void Struktur::System::WorldRenderSystem::Update(GameContext& context)
{
	World::World& world = context.GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	context.GetRideWorldRenderer().Render(context, world, context.GetTimeSystem().unscaledDelta);
}
