#include "WorldRenderSystem.h"

#include "Engine/GameContext.h"
#include "Renderer/WorldRenderer.h"
#include "RideContext.h"
#include "World/World.h"

void Struktur::System::WorldRenderSystem::Update(GameContext& context)
{
	World::World& world = Struktur::RideContext::From(context).GetWorld();
	if (!world.IsLoaded())
	{
		return;
	}
	Struktur::RideContext::From(context).GetWorldRenderer().Render(context, world,
	                                                               context.GetTimeSystem().unscaledDelta);
}
