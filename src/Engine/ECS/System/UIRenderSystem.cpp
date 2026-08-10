#include "UIRenderSystem.h"

#include "Engine/GameContext.h"
#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/UIRenderer.h"
#endif

void Struktur::System::UIRenderSystem::Update(GameContext& context)
{
#if !defined(PLATFORM_WEB)
	// Self-contained per-frame view setup, same pattern as WorldRenderer::Flush setting WorldViewId's own
	// transform rather than Game.cpp doing it centrally.
	context.GetUIRenderer().SetupView(context);
#endif
	UI::UIManager& uiManager = context.GetUIManager();
	uiManager.Render(context);
}
