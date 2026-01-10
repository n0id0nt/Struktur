#include "WrenStateSystem.h"

#include "Engine/GameContext.h"

void Struktur::System::WrenStateSystem::Update(GameContext& context)
{
	auto& stateManager = context.GetWrenStateManager();
	stateManager.Update(context);
}

void Struktur::System::WrenStateRenderSystem::Update(GameContext& context)
{
	auto& stateManager = context.GetWrenStateManager();
	stateManager.Render(context);
}
