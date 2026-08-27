#include "UIClip.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UIClip::UIClip(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                             const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize)
{
	m_focusable = false;
	SetBatchRoot(true);
}

void Struktur::UI::UIClip::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UIClip::Render(GameContext& context)
{
	// Re-issues the clip rect only when m_bounds actually moved/resized (m_visualDirty, set by
	// SetPosition/SetSize/SetAnchorPoint) - not on every scroll offset change (UIScroll's GetContentOffset()
	// doesn't touch m_visualDirty), since the viewport rect itself doesn't change while its content scrolls.
	if (m_visualDirty)
	{
		context.GetUIRenderer().SetBatchClip(m_batch, m_bounds);
		m_visualDirty = false;
	}

	RenderChildren(context);
}
