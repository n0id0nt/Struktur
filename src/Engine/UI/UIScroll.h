#pragma once

#include "Engine/UI/UIClip.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIScroll - Adds scrolling on top of UIClip's masking. Per-axis toggleable (SetVerticalScrollEnabled/
// SetHorizontalScrollEnabled, both default true); content size is derived each Update() tick from direct
// children's combined extents (no explicit re-layout - children position themselves as usual, e.g. a
// hand-built vertical list, and UIScroll just measures the resulting bounding box).
//
// Scroll offset is exposed to descendants via GetContentOffset() (see UIElement::GetPosition()'s parent-walk
// formula), so scrolling reuses the exact same position-propagation mechanism UIElement::UpdateBounds()'s
// recursive cascade already provides - not a bespoke second pathway.
//
// Two independent, optional behaviors layer on top of the offset itself:
//  - Camera-style deadzone focus-follow (mirrors Component::Camera/CameraSystem::CalculateSmoothedPosition) -
//    keeps the currently-focused descendant within an offset±deadzone band, lerping toward the band edge at
//    m_focusDamping's rate rather than snapping/re-centering every focus change.
//  - A two-way position sync with a plain UIElement (SetScrollIndicator) acting as the scrollbar thumb - see
//    UpdateScrollIndicator()'s own comment for the exact contract.
//=============================================================================
class UIScroll : public UIClip
{
public:
	UIScroll(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	         const glm::vec2& relativeSize);

	void Update(GameContext& context) override;

	void SetVerticalScrollEnabled(bool enabled)
	{
		m_verticalScrollEnabled = enabled;
	}
	void SetHorizontalScrollEnabled(bool enabled)
	{
		m_horizontalScrollEnabled = enabled;
	}
	bool IsVerticalScrollEnabled() const
	{
		return m_verticalScrollEnabled;
	}
	bool IsHorizontalScrollEnabled() const
	{
		return m_horizontalScrollEnabled;
	}

	// Clamps to [0, max(0, contentSize - viewportSize)] per axis and zeroes whichever axis is disabled - safe to
	// call with any value. Cascades a recursive UpdateBounds() (see UIElement.h) if the clamped result actually
	// changes anything, repositioning every descendant to match.
	void SetScrollOffset(const glm::vec2& offset);
	glm::vec2 GetScrollOffset() const
	{
		return m_scrollOffset;
	}
	// Combined extent of this element's direct children, recomputed every Update() tick - see UpdateContentSize().
	glm::vec2 GetContentSize() const
	{
		return m_contentSize;
	}

	// Slack band (around the viewport's own center) the focused descendant is allowed to drift within before
	// this element starts scrolling to follow it, and the per-axis lerp rate used while catching up - same
	// role/units as Component::Camera's deadZone/damping fields.
	void SetFocusDeadzone(const glm::vec2& deadzone)
	{
		m_focusDeadzone = deadzone;
	}
	void SetFocusDamping(const glm::vec2& damping)
	{
		m_focusDamping = damping;
	}

	// indicator can be any existing widget (most naturally a UIPanel acting as a scrollbar thumb), positioned
	// within its own parent's bounds (the "track") - see UpdateScrollIndicator()'s own comment for the full
	// position<->offset mapping and the two-way sync contract.
	void SetScrollIndicator(UIElement* indicator)
	{
		m_scrollIndicator      = indicator;
		m_hasWrittenIndicator  = false;
	}

	glm::vec2 GetContentOffset() const override
	{
		return -m_scrollOffset;
	}

private:
	void UpdateContentSize();
	// Clamps and applies `offset`, cascading UpdateBounds() only if the clamped result actually differs from the
	// current m_scrollOffset - the single write path every other scroll-offset mutator (focus-follow, indicator
	// sync, SetScrollOffset) funnels through, so clamping/zeroing-disabled-axes/change-detection lives in one
	// place.
	void ApplyScrollOffset(const glm::vec2& offset);
	void UpdateFocusFollow(GameContext& context);
	void UpdateScrollIndicator();

	glm::vec2 m_scrollOffset = glm::vec2(0.0f, 0.0f);
	glm::vec2 m_contentSize  = glm::vec2(0.0f, 0.0f);
	bool m_verticalScrollEnabled   = true;
	bool m_horizontalScrollEnabled = true;

	glm::vec2 m_focusDeadzone = glm::vec2(24.0f, 24.0f);
	glm::vec2 m_focusDamping  = glm::vec2(8.0f, 8.0f);

	UIElement* m_scrollIndicator      = nullptr;
	glm::vec2 m_lastSetIndicatorPos   = glm::vec2(0.0f, 0.0f);
	// False until the first real write to m_scrollIndicator - guards UpdateScrollIndicator()'s external-move
	// comparison from firing against a meaningless zero-initialized m_lastSetIndicatorPos on the very first tick
	// a new indicator is set (see SetScrollIndicator, which resets this).
	bool m_hasWrittenIndicator = false;
};
}  // namespace UI
}  // namespace Struktur
