#pragma once

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UITexture - Minimal textured rect (1 quad, no solid-color-only mode - see UIColor for that, no border - see
// UIBorder) - use as a child alongside UIColor/UIBorder when you only want one of those in isolation, rather
// than UIPanel's bundled all-3.
//=============================================================================
class UITexture : public UIElement
{
private:
	Resource::ResourcePtr<Resource::TextureResource> m_texture;
	Util::Color m_tint = Util::ColorWhite;

public:
	UITexture(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
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
	Util::Color GetTint() const
	{
		return m_tint;
	}
	bool HasTexture() const
	{
		return (bool)m_texture;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// Always exactly 1 quad - fixed, not optional/toggleable, so there's no shrink-then-stale-quad case to
	// guard against here (unlike UIPanel's background+border combination).
	uint32_t GetRequiredQuadCount() const override
	{
		return 1;
	}
};
}  // namespace UI
}  // namespace Struktur
