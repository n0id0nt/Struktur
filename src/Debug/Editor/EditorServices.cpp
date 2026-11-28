#include "EditorServices.h"

namespace Struktur::Debug
{
Services::Services()
{
	editor        = std::make_unique<Editor>();
	imGuiRenderer = std::make_unique<Renderer::ImGuiRenderer>();
}

void Services::Shutdown()
{
	editor.reset();
	imGuiRenderer->Shutdown();  // while the graphics device is still up
	imGuiRenderer.reset();
}
}  // namespace Struktur::Debug
