#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Callback/Callback.h"
#include "Engine/Renderer/UIBatchTypes.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"
#include "glm/glm.hpp"

namespace Struktur
{
class GameContext;

namespace Renderer
{
class UIRenderer;
}

namespace UI
{
// Enumerations
enum class NavigationDirection
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

class UIElement;

// Callback function types
using UIClickCallback    = Callback::TypedCallback<void(UIElement* sender, const glm::vec2& mousePos)>;
using UIFocusCallback    = Callback::TypedCallback<void(UIElement* sender)>;
using UIHoverCallback    = Callback::TypedCallback<void(UIElement* sender, const glm::vec2& mousePos)>;
using UIActivateCallback = Callback::TypedCallback<void(UIElement* sender)>;
using UIKeyCallback      = Callback::TypedCallback<void(UIElement* sender, int key)>;

//=============================================================================
// UIElement - Base class for all UI elements
//=============================================================================
class UIElement
{
	// Needs direct write access to m_batch/m_ownBatch/m_batchSlot (see AssignBatches) - kept protected rather
	// than exposed through public setters that any other caller could also reach for.
	friend class Renderer::UIRenderer;

public:
	UIElement(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	          const glm::vec2& relativeSize);
	virtual ~UIElement();

	// Delete copy constructor and copy assignment operator
	UIElement(const UIElement&)            = delete;
	UIElement& operator=(const UIElement&) = delete;

	// Allow move constructor and move assignment operator
	UIElement(UIElement&&)            = default;
	UIElement& operator=(UIElement&&) = default;

	virtual void Dispose(GameContext& context);

	// Pure virtual methods that must be implemented
	virtual void Update(GameContext& context) = 0;
	virtual void Render(GameContext& context) = 0;

	// Virtual methods with default implementations
	void OnClick(GameContext& context, const glm::vec2& mousePos);
	void OnHover(GameContext& context, const glm::vec2& mousePos);
	void OnFocus(GameContext& context);
	void OnLoseFocus(GameContext& context);
	void OnButtonPressed(GameContext& context, int key);
	void OnActivate(GameContext& context);

	// How many quads this element's own Render() draws (not counting children) - used by
	// UIRenderer::AssignBatches to size m_batchSlot. Default of 1 covers the common single-rect case; subclasses
	// with more (or variable, e.g. text-length-dependent) draw calls override this - see UIPanel/UILabel. Not a
	// hard cap: AllocateOrResizeSlot grows the slot later if a subsequent AssignBatches pass asks for more.
	virtual uint32_t GetRequiredQuadCount() const
	{
		return 1;
	}

	// Child management
	UIElement* AddChild(std::unique_ptr<UIElement> child);
	UIElement* AddChild(UIElement* child);
	bool RemoveChild(UIElement* child);

	// Position and size
	void SetPosition(const glm::vec2& absolutePosition, const glm::vec2& relativePosition);
	void SetSize(const glm::vec2& absoluteSize, const glm::vec2& relativeSize);
	void SetAnchorPoint(const glm::vec2& anchorPoint);

	glm::vec2 GetPosition() const;
	glm::vec2 GetSize() const;
	Util::Math::Rect GetBounds() const;

	// Additive offset a parent applies to every child's GetPosition() (e.g. a scroll container's current scroll
	// offset) - defaults to zero, so every existing widget is unaffected. Override in a subclass that needs to
	// reposition its whole subtree without touching each child's own m_absolutePosition/m_relativePosition, and
	// call UpdateBounds() when the value changes so the recursive cascade refreshes descendants' cached bounds.
	virtual glm::vec2 GetContentOffset() const
	{
		return glm::vec2(0.0f, 0.0f);
	}

	// Utility methods
	bool IsPointInside(const glm::vec2& point) const;

	// Visibility and state
	void SetVisible(bool vis)
	{
		m_visible = vis;
	}
	void SetEnabled(bool en)
	{
		m_enabled = en;
	}
	bool IsVisible() const
	{
		return m_visible;
	}
	bool IsEnabled() const
	{
		return m_enabled;
	}
	// IsVisible() plus a bounds-overlap check against the nearest clipping ancestor's current viewport (see
	// ClipsChildren) - an element scrolled/clipped out of view is IsVisible()==true (nothing toggled it off) but
	// not effectively visible. No clipping ancestor -> identical to IsVisible(). Used by FocusNavigator so
	// navigation skips elements the player can't actually see (see FocusNavigator.cpp).
	bool IsEffectivelyVisible() const;

	// True for elements that mask their children to their own current bounds (see UIClip) - checked by
	// IsEffectivelyVisible's ancestor walk. Default false so every existing widget is unaffected.
	virtual bool ClipsChildren() const
	{
		return false;
	}

	// Focus and navigation
	void SetFocusable(bool focus)
	{
		m_focusable = focus;
	}
	bool IsFocusable() const
	{
		return m_focusable;
	}
	void SetTabIndex(int index)
	{
		m_tabIndex = index;
	}
	int GetTabIndex() const
	{
		return m_tabIndex;
	}

	void SetNavigationNeighbor(NavigationDirection dir, UIElement* neighbor);
	UIElement* GetNavigationNeighbor(NavigationDirection dir) const;

	// Hierarchy
	UIElement* GetParent() const
	{
		return m_parent;
	}
	const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
	{
		return m_children;
	}
	// Walks up via m_parent to the top of the tree - used to re-run UIRenderer::AssignBatches from whatever
	// UIElement a mutation (AddChild/RemoveChild) happened on, since assignment must always start from the
	// tree's root (see WrenUI.cpp's addChild/removeChild bindings, the only place this is called today).
	UIElement* GetRoot()
	{
		UIElement* root = this;
		while (root->m_parent)
		{
			root = root->m_parent;
		}
		return root;
	}

	// Batching (see UIRenderer::AssignBatches, which is what actually sets these). Toggling this changes which
	// batch this element (and its whole subtree) should use, but doesn't re-run assignment itself - unlike
	// AddChild/RemoveChild, there's no existing call site for this yet to hook that into automatically, so
	// whoever calls this is responsible for following up with context.GetUIRenderer().AssignBatches(GetRoot(),
	// GetRoot()->GetBatch()) afterward (matching WrenUI.cpp's addChild/removeChild pattern) once this has a
	// real caller.
	void SetBatchRoot(bool isRoot)
	{
		m_isBatchRoot = isRoot;
	}
	bool IsBatchRoot() const
	{
		return m_isBatchRoot;
	}
	// The batch this element is currently assigned to (shared, or m_ownBatch if IsBatchRoot()) - only meaningful
	// after AssignBatches has run at least once; default-constructed (invalid) before that.
	Renderer::UIBatchHandle GetBatch() const
	{
		return m_batch;
	}

	// ID management
	void SetId(const std::string& elementId)
	{
		m_id = elementId;
	}
	const std::string& GetId() const
	{
		return m_id;
	}

	// Z-order - only reorders quads sharing a batch (see UIRenderer::DrawRect/quadZIndex); has no effect on
	// cross-batch order (UIBatch::drawOrder) unless this element is a batch root, in which case the Wren binding
	// (WrenUI.cpp's setZIndex) must also call UIRenderer::SetBatchDrawOrder - matching the "renderer-side state
	// needs an explicit follow-up call" contract SetBatchRoot's own comment already documents.
	void SetZIndex(int z)
	{
		m_zIndex      = z;
		m_visualDirty = true;
	}
	int GetZIndex() const
	{
		return m_zIndex;
	}

	// Callback setters - Fluent interface style
	UIElement* SetOnClick(UIClickCallback callback);
	UIElement* SetOnFocus(UIFocusCallback callback);
	UIElement* SetOnLoseFocus(UIFocusCallback callback);
	UIElement* SetOnHover(UIHoverCallback callback);
	UIElement* SetOnKeyPressed(UIKeyCallback callback);
	UIElement* SetOnActivate(UIActivateCallback callback);

	void ForEachRecursive(std::function<void(UIElement*)> func);
	void ForEachRecursivePostOrder(std::function<void(UIElement*)> func);

protected:
	void UpdateChildren(GameContext& context);
	void RenderChildren(GameContext& context);

	void UpdateBounds();

	bool m_disposed;

	Util::Math::Rect m_bounds;
	glm::vec2 m_absolutePosition;
	glm::vec2 m_relativePosition;
	glm::vec2 m_absoluteSize;
	glm::vec2 m_relativeSize;
	glm::vec2 m_anchorPoint;
	bool m_visible;
	bool m_enabled;
	bool m_focusable;
	UIElement* m_parent;
	std::vector<std::unique_ptr<UIElement>> m_children;
	std::string m_id;
	int m_zIndex;
	int m_tabIndex;
	std::vector<UIElement*> m_navigationNeighbors[4];  // UP, DOWN, LEFT, RIGHT

	// Batching (see UIRenderer::AssignBatches - phase 3 of the UI batching migration; nothing renders through
	// these yet, see UIRenderer's class comment). Written by UIRenderer (a friend, see above), not this class.
	Renderer::UIBatchHandle m_batch;
	Renderer::UIBatchHandle m_ownBatch;  // only valid/meaningful if m_isBatchRoot
	Renderer::UIBatchSlot m_batchSlot;
	bool m_isBatchRoot = false;
	// Set whenever this element's own drawable content changes (position/size/text/z-index/...; color/border on
	// whichever derived class owns those - see UIPanel/UIColor/UITexture/UIBorder) - read by each derived
	// class's own Render() to decide when a DrawX re-write, as opposed to just re-Flush, is needed.
	// Defaults true so a freshly constructed element's first real write isn't skipped as "unchanged".
	bool m_visualDirty = true;

	// Callback functions
	UIClickCallback m_onClickCallback;
	UIFocusCallback m_onFocusCallback;
	UIFocusCallback m_onLoseFocusCallback;
	UIHoverCallback m_onHoverCallback;
	UIActivateCallback m_onActivateCallback;
	UIKeyCallback m_onKeyPressedCallback;
};
}  // namespace UI
}  // namespace Struktur
