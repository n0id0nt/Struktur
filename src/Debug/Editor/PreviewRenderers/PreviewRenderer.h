#pragma once

#include <imgui.h>

#include <string>

namespace Struktur
{
class GameContext;
}

namespace Struktur::Debug
{
// Base class for all preview renderers
class IPreviewRenderer
{
   public:
	virtual ~IPreviewRenderer() = default;

	// Render the preview content
	virtual void Render(GameContext& context, const ImVec2& availableSize) = 0;

	// Get the name of this preview
	virtual std::string GetPreviewName() const = 0;

	// Render toolbar/controls (optional)
	virtual void RenderControls(GameContext& context) {}

	// Clear/reset the preview
	virtual void Clear() {}
};
}  // namespace Struktur::Debug
