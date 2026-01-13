#include "UIRenderSystem.h"

#include "Engine/GameContext.h"
#include "Debug/Profiling/Profiler.h"

void Struktur::System::UIRenderSystem::Update(GameContext &context)
{
    PROFILE_FUNCTION();
    UI::UIManager& uiManager = context.GetUIManager();
    uiManager.Render(context);
}
