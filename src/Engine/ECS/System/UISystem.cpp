#include "UISystem.h"

#include "Engine/GameContext.h"

void Struktur::System::UISystem::Update(GameContext &context)
{
    UI::UIManager& uiManager = context.GetUIManager();
    uiManager.Update(context);
}

void Struktur::System::UIRenderSystem::Update(GameContext &context)
{
    UI::UIManager& uiManager = context.GetUIManager();
    uiManager.Render(context);
}
