#pragma once

#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIColor - Minimal solid-color rect (1 quad, no background texture, no border) - use as a child alongside
// UITexture/UIBorder when you only want one of those in isolation, rather than UIPanel's bundled all-3.
//=============================================================================
class UIColor : public UIElement
{
private:
	Util::Color m_color = Util::ColorWhite;

public:
	UIColor(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	        const glm::vec2& relativeSize);

	void SetColor(Util::Color color)
	{
		m_color = color;
		m_visualDirty = true;
	}
	Util::Color GetColor() const
	{
		return m_color;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// Always exactly 1 quad - fixed, not optional/toggleable like UIPanel's background+border combination, so
	// there's no shrink-then-stale-quad case to guard against here.
	uint32_t GetRequiredQuadCount() const override
	{
		return 1;
	}
};
}  // namespace UI
}  // namespace Struktur
