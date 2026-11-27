#include "WorldSystem.h"

#include "Engine/GameContext.h"

void Struktur::System::WorldSystem::Update(GameContext& context)
{
	World::World& world = context.GetWorld();
	if (world.IsLoaded())
	{
		world.Update(context.GetTimeSystem().unscaledDelta, context.GetInput().GetTrainer());
	}
}
