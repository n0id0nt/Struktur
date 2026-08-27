#include "UIScroll.h"

#include <algorithm>

#include "Engine/GameContext.h"
#include "Engine/UI/FocusNavigator.h"
#include "Engine/UI/UIManager.h"
#include "Engine/Util/MathUtil.h"

Struktur::UI::UIScroll::UIScroll(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                                 const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIClip(absolutePosition, relativePosition, absoluteSize, relativeSize)
{
}

void Struktur::UI::UIScroll::Update(GameContext& context)
{
	UpdateContentSize();
	// Re-clamps against this frame's content size even if nothing asked to scroll - covers content shrinking
	// (e.g. a child removed) leaving m_scrollOffset pointing past the new max.
	ApplyScrollOffset(m_scrollOffset);
	UpdateFocusFollow(context);
	UpdateScrollIndicator();

	UpdateChildren(context);
}

void Struktur::UI::UIScroll::UpdateContentSize()
{
	glm::vec2 viewportPos = GetPosition();
	glm::vec2 offset      = GetContentOffset();
	glm::vec2 maxExtent   = glm::vec2(0.0f, 0.0f);

	for (auto& child : m_children)
	{
		// Subtracting this element's own current content offset recovers the child's pre-scroll local position,
		// so the measured content size doesn't shrink/grow as a side effect of scrolling itself.
		glm::vec2 childLocalPos = child->GetPosition() - offset - viewportPos;
		glm::vec2 childSize     = child->GetSize();
		maxExtent.x             = std::max(maxExtent.x, childLocalPos.x + childSize.x);
		maxExtent.y             = std::max(maxExtent.y, childLocalPos.y + childSize.y);
	}

	m_contentSize = maxExtent;
}

void Struktur::UI::UIScroll::SetScrollOffset(const glm::vec2& offset)
{
	ApplyScrollOffset(offset);
}

void Struktur::UI::UIScroll::ApplyScrollOffset(const glm::vec2& offset)
{
	glm::vec2 maxOffset = glm::max(glm::vec2(0.0f, 0.0f), m_contentSize - GetSize());
	glm::vec2 clamped   = glm::clamp(offset, glm::vec2(0.0f, 0.0f), maxOffset);
	if (!m_verticalScrollEnabled)
	{
		clamped.y = 0.0f;
	}
	if (!m_horizontalScrollEnabled)
	{
		clamped.x = 0.0f;
	}

	if (clamped != m_scrollOffset)
	{
		m_scrollOffset = clamped;
		// Cascades to every descendant via the recursive UpdateBounds() (see UIElement.cpp) - reuses the exact
		// mechanism a parent SetPosition/SetSize/SetAnchorPoint call already needed for its own children.
		UpdateBounds();
	}
}

void Struktur::UI::UIScroll::UpdateFocusFollow(GameContext& context)
{
	UIElement* focused = context.GetUIManager().GetFocusNavigator()->GetCurrentFocus();
	if (!focused)
	{
		return;
	}

	bool isDescendant = false;
	ForEachRecursive(
	    [&](UIElement* elem)
	    {
		    if (elem == focused)
		    {
			    isDescendant = true;
		    }
	    });
	if (!isDescendant)
	{
		return;
	}

	float deltaTime = context.GetTimeSystem().scaledDelta;

	glm::vec2 viewportCenter = GetPosition() + GetSize() * 0.5f;
	glm::vec2 focusedCenter  = focused->GetPosition() + focused->GetSize() * 0.5f;

	glm::vec2 newOffset = m_scrollOffset;

	// x - mirrors CameraSystem::CalculateSmoothedPosition's own offset±deadZone band check, just driving a
	// scroll offset instead of a camera position.
	if (m_horizontalScrollEnabled)
	{
		float delta = focusedCenter.x - viewportCenter.x;
		if (delta > m_focusDeadzone.x)
		{
			newOffset.x = Util::Math::Lerp(m_scrollOffset.x, m_scrollOffset.x + (delta - m_focusDeadzone.x),
			                               m_focusDamping.x * deltaTime);
		}
		else if (delta < -m_focusDeadzone.x)
		{
			newOffset.x = Util::Math::Lerp(m_scrollOffset.x, m_scrollOffset.x + (delta + m_focusDeadzone.x),
			                               m_focusDamping.x * deltaTime);
		}
	}
	// y
	if (m_verticalScrollEnabled)
	{
		float delta = focusedCenter.y - viewportCenter.y;
		if (delta > m_focusDeadzone.y)
		{
			newOffset.y = Util::Math::Lerp(m_scrollOffset.y, m_scrollOffset.y + (delta - m_focusDeadzone.y),
			                               m_focusDamping.y * deltaTime);
		}
		else if (delta < -m_focusDeadzone.y)
		{
			newOffset.y = Util::Math::Lerp(m_scrollOffset.y, m_scrollOffset.y + (delta + m_focusDeadzone.y),
			                               m_focusDamping.y * deltaTime);
		}
	}

	ApplyScrollOffset(newOffset);
}

void Struktur::UI::UIScroll::UpdateScrollIndicator()
{
	if (!m_scrollIndicator)
	{
		return;
	}

	UIElement* track = m_scrollIndicator->GetParent();
	if (!track)
	{
		return;
	}

	// Drives whichever axis is enabled - vertical takes priority when both are (the common single-scrollbar
	// case); a horizontal-only UIScroll drives the indicator along X instead.
	bool useVertical = m_verticalScrollEnabled;

	glm::vec2 trackSize   = track->GetSize();
	float trackExtent     = useVertical ? trackSize.y : trackSize.x;
	float viewportExtent  = useVertical ? GetSize().y : GetSize().x;
	float contentExtent   = useVertical ? m_contentSize.y : m_contentSize.x;
	float maxOffset       = std::max(0.0f, contentExtent - viewportExtent);
	float currentOffset   = useVertical ? m_scrollOffset.y : m_scrollOffset.x;

	float thumbRatio  = contentExtent > 0.0f ? std::clamp(viewportExtent / contentExtent, 0.0f, 1.0f) : 1.0f;
	float thumbExtent = trackExtent * thumbRatio;
	float scrollT     = maxOffset > 0.0f ? currentOffset / maxOffset : 0.0f;
	float thumbPos    = (trackExtent - thumbExtent) * scrollT;

	glm::vec2 computedPos  = useVertical ? glm::vec2(0.0f, thumbPos) : glm::vec2(thumbPos, 0.0f);
	glm::vec2 computedSize = useVertical ? glm::vec2(trackSize.x, thumbExtent) : glm::vec2(thumbExtent, trackSize.y);

	if (m_hasWrittenIndicator)
	{
		glm::vec2 actualPos = m_scrollIndicator->GetPosition() - track->GetPosition();
		// Compared against what THIS element last wrote there (not just "did it move") to avoid a feedback loop -
		// see the class comment/plan for why. No live trigger for this branch in v1 (no mouse-drag or indicator-
		// nudge input source exists yet) - fires for free once one does.
		if (glm::length(actualPos - m_lastSetIndicatorPos) > 0.5f)
		{
			float availableTravel = trackExtent - thumbExtent;
			float movedT = availableTravel > 0.0f ? (useVertical ? actualPos.y : actualPos.x) / availableTravel : 0.0f;
			float newAxisOffset   = std::clamp(movedT, 0.0f, 1.0f) * maxOffset;

			glm::vec2 newScrollOffset = m_scrollOffset;
			if (useVertical)
			{
				newScrollOffset.y = newAxisOffset;
			}
			else
			{
				newScrollOffset.x = newAxisOffset;
			}
			ApplyScrollOffset(newScrollOffset);
			// Don't immediately overwrite the externally-set position this same frame - let the next tick's
			// recomputed thumbPos (from the now-updated m_scrollOffset) reconcile it instead.
			return;
		}
	}

	m_scrollIndicator->SetPosition(computedPos, glm::vec2(0.0f, 0.0f));
	m_scrollIndicator->SetSize(computedSize, glm::vec2(0.0f, 0.0f));
	m_lastSetIndicatorPos = computedPos;
	m_hasWrittenIndicator = true;
}
