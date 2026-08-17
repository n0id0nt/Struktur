#pragma once

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIPanel - Container element
//=============================================================================
class UIPanel : public UIElement
{
private:
	Resource::ResourcePtr<Resource::TextureResource> m_backgroundTexture;
	bool m_hasBackgroundTexture;
	// Bundled here rather than on the shared UIElement base - a panel commonly wants some combination of these
	// three together (matching e.g. WPF's Border element), but plenty of other UIElement subclasses (UILabel,
	// UIColor, UITexture, UIBorder) don't need all three sitting on them unused. Use UIColor/UITexture/UIBorder
	// as separate children instead when you only want one of these in isolation.
	Util::Color m_backgroundColor = Util::ColorLightGray;
	Util::Color m_borderColor     = Util::ColorDarkGray;
	float m_borderWidth           = 1.0f;

public:
	UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	        const glm::vec2& relativeSize);

	void SetBackgroundTexture(const Resource::ResourcePtr<Resource::TextureResource>& texture);
	void ClearBackgroundTexture();

	void SetBackgroundColor(Util::Color color)
	{
		m_backgroundColor = color;
		m_visualDirty     = true;
	}
	void SetBorderColor(Util::Color color)
	{
		m_borderColor = color;
		m_visualDirty = true;
	}
	void SetBorderWidth(float width)
	{
		m_borderWidth = width;
		m_visualDirty = true;
	}
	Util::Color GetBackgroundColor() const
	{
		return m_backgroundColor;
	}
	Util::Color GetBorderColor() const
	{
		return m_borderColor;
	}
	float GetBorderWidth() const
	{
		return m_borderWidth;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// 1 quad for the background (DrawRect/DrawTexturedRect) plus 4 for the border outline (DrawRectOutline) when
	// one is actually drawn - mirrors Render()'s own draw calls exactly.
	uint32_t GetRequiredQuadCount() const override;
};
}  // namespace UI
}  // namespace Struktur
