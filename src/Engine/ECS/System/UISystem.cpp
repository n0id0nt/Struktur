#include "UISystem.h"

#include "Engine/GameContext.h"
#include "Engine/UI/UIManager.h"

void Struktur::System::UISystem::Update(GameContext& context)
{
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Update(context);
}
