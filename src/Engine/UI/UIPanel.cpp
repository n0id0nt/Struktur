#include "UIPanel.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UIPanel::UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                               const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize),
      m_hasBackgroundTexture(false)
{
	m_backgroundColor = Util::ColorLightGray;
	m_focusable       = false;  // Panels typically don't receive focus
}

void Struktur::UI::UIPanel::SetBackgroundTexture(const Resource::ResourcePtr<Resource::TextureResource>& texture)
{
	m_backgroundTexture    = texture;
	m_hasBackgroundTexture = true;
}

void Struktur::UI::UIPanel::ClearBackgroundTexture()
{
	m_hasBackgroundTexture = false;
	m_backgroundTexture    = Resource::ResourcePtr<Resource::TextureResource>();
}

void Struktur::UI::UIPanel::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UIPanel::Render(GameContext& context)
{
	if (m_hasBackgroundTexture)
	{
		Resource::TextureResource* texture = m_backgroundTexture.Get();
		if (!texture->IsGpuReady())
		{
			texture->LoadToGpu(context);
		}
		context.GetUIRenderer().DrawTexturedRect(m_bounds, texture->GetHandle(), Util::ColorWhite);
	}
	else
	{
		context.GetUIRenderer().DrawRect(m_bounds, m_backgroundColor);
	}

	if (m_borderWidth > 0)
	{
		context.GetUIRenderer().DrawRectOutline(m_bounds, m_borderWidth, m_borderColor);
	}

	RenderChildren(context);
}
