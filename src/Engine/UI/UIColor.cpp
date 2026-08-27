#include "UIColor.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UIColor::UIColor(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                               const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize)
{
	m_focusable = false;
}

void Struktur::UI::UIColor::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UIColor::Render(GameContext& context)
{
	if (m_visualDirty)
	{
		context.GetUIRenderer().DrawRect(m_batch, m_batchSlot, m_bounds, m_color, GetZIndex());
		m_visualDirty = false;
	}

	RenderChildren(context);
}
