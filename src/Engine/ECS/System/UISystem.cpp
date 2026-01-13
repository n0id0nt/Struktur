#include "UISystem.h"

#include "Engine/GameContext.h"
#include "Debug/Profiling/Profiler.h"

void Struktur::System::UISystem::Update(GameContext &context)
{
    PROFILE_FUNCTION();
    UI::UIManager& uiManager = context.GetUIManager();
    uiManager.Update(context);
}
