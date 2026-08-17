#pragma once

#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIBorder - Minimal border outline (4 quads, no background of any kind - see UIColor/UITexture for that) - use
// as a child alongside UIColor/UITexture when you only want one of those in isolation, rather than UIPanel's
// bundled all-3.
//=============================================================================
class UIBorder : public UIElement
{
private:
	Util::Color m_color = Util::ColorDarkGray;
	float m_width       = 1.0f;

public:
	UIBorder(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	         const glm::vec2& relativeSize);

	void SetColor(Util::Color color)
	{
		m_color = color;
		m_visualDirty = true;
	}
	void SetWidth(float width)
	{
		m_width = width;
		m_visualDirty = true;
	}
	Util::Color GetColor() const
	{
		return m_color;
	}
	float GetWidth() const
	{
		return m_width;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// Always exactly 4 quads, regardless of SetWidth - a width of 0 still writes 4 (now zero-thickness, so
	// effectively invisible) quads rather than shrinking the slot, so there's no shrink-then-stale-quad case to
	// guard against here the way UIPanel's own border needs ClearSlotFrom for.
	uint32_t GetRequiredQuadCount() const override
	{
		return 4;
	}
};
}  // namespace UI
}  // namespace Struktur
