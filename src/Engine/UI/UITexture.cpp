#include "UITexture.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UITexture::UITexture(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                                   const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize)
{
	m_focusable = false;
}

void Struktur::UI::UITexture::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UITexture::Render(GameContext& context)
{
	if (m_visualDirty)
	{
		if (m_texture)
		{
			Resource::TextureResource* texture = m_texture.Get();
			if (!texture->IsGpuReady())
			{
				texture->LoadToGpu(context);
			}
			context.GetUIRenderer().DrawTexturedRect(m_batch, m_batchSlot, m_bounds, texture->GetHandle(), m_tint,
			                                         GetZIndex());
		}
		m_visualDirty = false;
	}

	RenderChildren(context);
}
