#include "UIRenderSystem.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/UI/UIManager.h"

void Struktur::System::UIRenderSystem::Update(GameContext& context)
{
	// Self-contained per-frame view setup: each renderer sets its own views' transforms.
	context.GetUIRenderer().SetupView(context);
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Render(context);

	// All widget Render() calls above have written their dirty quads into their batch slots (see UIElement's
	// class comment) - Flush uploads whatever's dirty and submits every active batch, once per frame.
	context.GetUIRenderer().Flush(context);
}
