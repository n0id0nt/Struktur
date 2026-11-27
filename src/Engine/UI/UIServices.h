#pragma once

#include <memory>

#include "Engine/Renderer/UIRenderer.h"
#include "Engine/UI/InputGlyphs.h"
#include "Engine/UI/UIManager.h"

namespace Struktur::UI
{
// The UI library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<InputGlyphs> inputGlyphs;
	std::unique_ptr<UIManager> uiManager;
	std::unique_ptr<Renderer::UIRenderer> uiRenderer;
};
}  // namespace Struktur::UI
