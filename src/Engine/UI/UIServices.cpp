#include "UIServices.h"

namespace Struktur::UI
{
Services::Services()
{
	inputGlyphs = std::make_unique<InputGlyphs>();
	uiManager   = std::make_unique<UIManager>();
	uiRenderer  = std::make_unique<Renderer::UIRenderer>();
}

void Services::Shutdown()
{
	inputGlyphs.reset();  // holds texture ResourcePtrs - must go before the resource manager
	uiManager.reset();
	uiRenderer->Shutdown();  // while the graphics device is still up
	uiRenderer.reset();
}
}  // namespace Struktur::UI
