#pragma once

#include "PreviewRenderer.h"

namespace Struktur
{
namespace Component
{
struct Sprite;
}
}  // namespace Struktur

namespace Struktur::Debug
{
class SpritePreviewRenderer : public IPreviewRenderer
{
public:
	// Which frame(s) of the sprite sheet Render() draws - see RenderControls for the toggle UI and Render's own
	// per-mode branch.
	enum class Mode
	{
		CurrentSprite,    // sprite->index - whatever frame the component is actually showing right now
		SpecifiedSprite,  // m_specifiedIndex - a user-picked frame, for inspecting frames the entity isn't on
		EntireSheet       // the whole texture with a column/row grid overlay and the current frame highlighted
	};

	SpritePreviewRenderer(const Component::Sprite* sprite, const std::string& name)
	    : m_sprite(sprite),
	      m_name(name)
	{
	}

	void RenderControls(GameContext& context) override;
	void Render(GameContext& context, const ImVec2& availableSize) override;
	std::string GetPreviewName() const override
	{
		return m_name;
	}

private:
	const Component::Sprite* m_sprite;
	std::string m_name;
	Mode m_mode          = Mode::CurrentSprite;
	int m_specifiedIndex = 0;

	// Shared by CurrentSprite/SpecifiedSprite - draws one frame (whichever `index` names) scaled to fit
	// availableSize, letterboxed/centered the same way TexturePreviewRenderer's whole-image display is.
	void RenderSingleFrame(GameContext& context, const ImVec2& availableSize, int index);

	// EntireSheet mode - draws the full texture scaled to fit, with grid lines at every column/row boundary and
	// sprite->index's cell outlined, so a misconfigured columns/rows/offset is visible directly against the
	// actual sheet instead of having to reason about it from the numbers alone.
	void RenderWholeSheet(GameContext& context, const ImVec2& availableSize);
};
}  // namespace Struktur::Debug
