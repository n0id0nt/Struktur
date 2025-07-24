#include "GameplaySystem.h"

#include "Engine/GameContext.h"

void Struktur::System::GameplaySystem::Update(GameContext &context)
{
    auto& stateManager = context.GetStateManager();
    stateManager.Update(context);
}

void Struktur::System::GameplayRenderSystem::Update(GameContext &context)
{
    auto& stateManager = context.GetStateManager();
    stateManager.Render(context);
}
