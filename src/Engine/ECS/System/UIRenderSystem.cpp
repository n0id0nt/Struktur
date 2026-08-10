#include "UIRenderSystem.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

void Struktur::System::UIRenderSystem::Update(GameContext& context)
{
	// Self-contained per-frame view setup, same pattern as WorldRenderer::Flush setting WorldViewId's own
	// transform rather than Game.cpp doing it centrally.
	context.GetUIRenderer().SetupView(context);
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Render(context);
}
