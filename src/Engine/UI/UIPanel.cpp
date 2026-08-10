#include "UIPanel.h"

#include "Engine/GameContext.h"
#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/UIRenderer.h"
#endif

Struktur::UI::UIPanel::UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                               const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize),
      m_hasBackgroundTexture(false)
{
	m_backgroundColor = LIGHTGRAY;
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
#if defined(PLATFORM_WEB)
	// Draw background
	if (m_hasBackgroundTexture)
	{
		// Scale texture to fit panel
		Resource::TextureResource* texture = m_backgroundTexture.Get();
		if (!texture->IsGpuReady())
		{
			texture->LoadToGpu(context);
		}
		::Rectangle srcRect = {0, 0, (float)texture->GetWidth(), (float)texture->GetHeight()};
		::DrawTexturePro(texture->texture, srcRect, m_bounds, {0, 0}, 0.0f, WHITE);
	}
	else
	{
		::DrawRectangleRec(m_bounds, m_backgroundColor);
	}

	// Draw border
	if (m_borderWidth > 0)
	{
		::DrawRectangleLinesEx(m_bounds, m_borderWidth, m_borderColor);
	}
#else
	if (m_hasBackgroundTexture)
	{
		Resource::TextureResource* texture = m_backgroundTexture.Get();
		if (!texture->IsGpuReady())
		{
			texture->LoadToGpu(context);
		}
		context.GetUIRenderer().DrawTexturedRect(m_bounds, texture->GetHandle(), WHITE);
	}
	else
	{
		context.GetUIRenderer().DrawRect(m_bounds, m_backgroundColor);
	}

	if (m_borderWidth > 0)
	{
		context.GetUIRenderer().DrawRectOutline(m_bounds, m_borderWidth, m_borderColor);
	}
#endif

	RenderChildren(context);
}
