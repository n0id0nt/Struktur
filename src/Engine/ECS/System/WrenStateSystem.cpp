#include "WrenStateSystem.h"

#include "Engine/GameContext.h"
#include "Debug/Profiling/Profiler.h"

void Struktur::System::WrenStateSystem::Update(GameContext& context)
{
	PROFILE_FUNCTION();
	auto& stateManager = context.GetWrenStateManager();
	stateManager.Update(context);
}

void Struktur::System::WrenStateRenderSystem::Update(GameContext& context)
{
	PROFILE_FUNCTION();
	auto& stateManager = context.GetWrenStateManager();
	stateManager.Render(context);
}
