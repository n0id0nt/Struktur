#pragma once

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UINineSlice - a texture drawn as a classic nine-slice/9-patch (see Renderer::NineSliceBorder): the four
// corners keep their native source-texture pixel size on screen regardless of this element's own size, the four
// edges stretch along one axis to fill the gap between corners, and the center stretches along both to fill the
// remainder - so a border/panel-style texture can be resized to any target size without its corner/edge art
// visibly distorting. Use this instead of UITexture whenever the source art has a border design meant to frame
// arbitrary content rather than stretch uniformly.
//=============================================================================
class UINineSlice : public UIElement
{
private:
	Resource::ResourcePtr<Resource::TextureResource> m_texture;
	Util::Color m_tint = Util::ColorWhite;
	float m_borderLeft = 0.0f;
	float m_borderRight = 0.0f;
	float m_borderTop = 0.0f;
	float m_borderBottom = 0.0f;

public:
	UINineSlice(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	            const glm::vec2& relativeSize);

	void SetTexture(const Resource::ResourcePtr<Resource::TextureResource>& texture)
	{
		m_texture = texture;
		m_visualDirty = true;
	}
	void SetTint(Util::Color tint)
	{
		m_tint = tint;
		m_visualDirty = true;
	}
	// Border insets in SOURCE TEXTURE pixel space (not this element's own size) - see Renderer::NineSliceBorder.
	void SetBorder(float left, float right, float top, float bottom)
	{
		m_borderLeft = left;
		m_borderRight = right;
		m_borderTop = top;
		m_borderBottom = bottom;
		m_visualDirty = true;
	}
	Util::Color GetTint() const
	{
		return m_tint;
	}
	float GetBorderLeft() const
	{
		return m_borderLeft;
	}
	float GetBorderRight() const
	{
		return m_borderRight;
	}
	float GetBorderTop() const
	{
		return m_borderTop;
	}
	float GetBorderBottom() const
	{
		return m_borderBottom;
	}
	bool HasTexture() const
	{
		return (bool)m_texture;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// Always exactly 9 quads (4 corners + 4 edges + center) - fixed, not conditional, so there's no shrink-then-
	// stale-quad case to guard against here.
	uint32_t GetRequiredQuadCount() const override
	{
		return 9;
	}
};
}  // namespace UI
}  // namespace Struktur
