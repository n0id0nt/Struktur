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

	virtual void RenderFocusIndicator(GameContext& context);

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

	// Styling
	void SetBackgroundColor(Util::Color color)
	{
		m_backgroundColor = color;
		m_visualDirty     = true;
	}
	void SetBorderColor(Util::Color color)
	{
		m_borderColor = color;
		m_visualDirty = true;
	}
	void SetBorderWidth(float width)
	{
		m_borderWidth = width;
		m_visualDirty = true;
	}

	Util::Color GetBackgroundColor() const
	{
		return m_backgroundColor;
	}
	Util::Color GetBorderColor() const
	{
		return m_borderColor;
	}
	float GetBorderWidth() const
	{
		return m_borderWidth;
	}

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

	// Z-order
	void SetZIndex(int z)
	{
		m_zIndex = z;
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
	Util::Color m_backgroundColor;
	Util::Color m_borderColor;
	float m_borderWidth;
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
	// Set whenever this element's own drawable content changes (position/size/color/text/...) - not yet read by
	// anything (phase 4 will use it to decide when a WriteX re-write, as opposed to just re-Flush, is needed).
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
