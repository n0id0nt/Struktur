#include "UINineSlice.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UINineSlice::UINineSlice(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                                       const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize)
{
	m_focusable = false;
}

void Struktur::UI::UINineSlice::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UINineSlice::Render(GameContext& context)
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
			Renderer::NineSliceBorder border{m_borderLeft, m_borderRight, m_borderTop, m_borderBottom};
			context.GetUIRenderer().DrawNineSlice(m_batch, m_batchSlot, m_bounds, texture->GetHandle(), border,
			                                      m_tint, GetZIndex());
		}
		m_visualDirty = false;
	}

	RenderChildren(context);
}
