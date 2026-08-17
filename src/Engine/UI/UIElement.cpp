#include "UIElement.h"

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UIElement::UIElement(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                                   const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : m_absolutePosition(absolutePosition),
      m_relativePosition(relativePosition),
      m_absoluteSize(absoluteSize),
      m_relativeSize(relativeSize),
      m_anchorPoint(glm::vec2()),
      m_backgroundColor(Util::ColorLightGray),
      m_borderColor(Util::ColorDarkGray),
      m_borderWidth(1.0f),
      m_visible(true),
      m_enabled(true),
      m_focusable(false),
      m_parent(nullptr),
      m_zIndex(0),
      m_tabIndex(-1),
      m_disposed(false)
{
	UpdateBounds();
}

Struktur::UI::UIElement::~UIElement()
{
	ASSERT_MSG(m_disposed, "UI Element has not been disposed");
}

Struktur::UI::UIElement* Struktur::UI::UIElement::AddChild(std::unique_ptr<UIElement> child)
{
	if (child)
	{
		child->m_parent     = this;
		UIElement* childPtr = child.get();
		m_children.push_back(std::move(child));
		childPtr->UpdateBounds();
		return childPtr;
	}
	return nullptr;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::AddChild(UIElement* child)
{
	if (child)
	{
		child->m_parent = this;
		m_children.push_back(std::unique_ptr<UIElement>(child));
		child->UpdateBounds();
		return child;
	}
	return nullptr;
}

bool Struktur::UI::UIElement::RemoveChild(UIElement* child)
{
	auto it = std::find_if(m_children.begin(), m_children.end(),
	                       [child](const std::unique_ptr<UIElement>& ptr) { return ptr.get() == child; });

	if (it != m_children.end())
	{
		child->UpdateBounds();
		m_children.erase(it);
		return true;
	}
	return false;
}

void Struktur::UI::UIElement::SetPosition(const glm::vec2& absolutePosition, const glm::vec2& relativePosition)
{
	m_absolutePosition = absolutePosition;
	m_relativePosition = relativePosition;
	UpdateBounds();
	m_visualDirty = true;
}

void Struktur::UI::UIElement::SetSize(const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
{
	m_absoluteSize = absoluteSize;
	m_relativeSize = relativeSize;
	UpdateBounds();
	m_visualDirty = true;
}

void Struktur::UI::UIElement::SetAnchorPoint(const glm::vec2& anchorPoint)
{
	m_anchorPoint = anchorPoint;
	UpdateBounds();
	m_visualDirty = true;
}

glm::vec2 Struktur::UI::UIElement::GetPosition() const
{
	glm::vec2 basePosition = glm::vec2();
	glm::vec2 baseSize     = glm::vec2();
	if (m_parent)
	{
		basePosition = m_parent->GetPosition();
		baseSize     = m_parent->GetSize();
	}

	glm::vec2 absolutePosition = m_absolutePosition;
	glm::vec2 relativePosition = baseSize * m_relativePosition;
	glm::vec2 anchorOffset     = m_anchorPoint * GetSize();

	return basePosition + absolutePosition + relativePosition - anchorOffset;
}

glm::vec2 Struktur::UI::UIElement::GetSize() const
{
	glm::vec2 basePosition = glm::vec2();
	glm::vec2 baseSize     = glm::vec2();
	if (m_parent)
	{
		basePosition = m_parent->GetPosition();
		baseSize     = m_parent->GetSize();
	}

	glm::vec2 absoluteSize = m_absoluteSize;
	glm::vec2 relativeSize = baseSize * m_relativeSize;

	return absoluteSize + relativeSize;
}

Struktur::Util::Math::Rect Struktur::UI::UIElement::GetBounds() const
{
	return m_bounds;
}

bool Struktur::UI::UIElement::IsPointInside(const glm::vec2& point) const
{
	return point.x >= m_bounds.x && point.x <= m_bounds.x + m_bounds.width && point.y >= m_bounds.y &&
	       point.y <= m_bounds.y + m_bounds.height;
}

void Struktur::UI::UIElement::SetNavigationNeighbor(NavigationDirection dir, UIElement* neighbor)
{
	m_navigationNeighbors[static_cast<int>(dir)].clear();
	if (neighbor)
	{
		m_navigationNeighbors[static_cast<int>(dir)].push_back(neighbor);
	}
}

Struktur::UI::UIElement* Struktur::UI::UIElement::GetNavigationNeighbor(NavigationDirection dir) const
{
	int dirIndex = static_cast<int>(dir);
	if (!m_navigationNeighbors[dirIndex].empty())
	{
		return m_navigationNeighbors[dirIndex][0];
	}
	return nullptr;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::SetOnClick(UIClickCallback callback)
{
	m_onClickCallback = std::move(callback);
	return this;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::SetOnFocus(UIFocusCallback callback)
{
	m_onFocusCallback = std::move(callback);
	return this;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::SetOnLoseFocus(UIFocusCallback callback)
{
	m_onLoseFocusCallback = std::move(callback);
	return this;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::SetOnHover(UIHoverCallback callback)
{
	m_onHoverCallback = std::move(callback);
	return this;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::SetOnKeyPressed(UIKeyCallback callback)
{
	m_onKeyPressedCallback = std::move(callback);
	return this;
}

Struktur::UI::UIElement* Struktur::UI::UIElement::SetOnActivate(UIActivateCallback callback)
{
	m_onActivateCallback = std::move(callback);
	return this;
}

void Struktur::UI::UIElement::ForEachRecursive(std::function<void(UIElement*)> func)
{
	func(this);  // Apply to self first

	for (auto& child : m_children)
	{
		child->ForEachRecursive(func);  // Recurse to children
	}
}

void Struktur::UI::UIElement::ForEachRecursivePostOrder(std::function<void(UIElement*)> func)
{
	for (auto& child : m_children)
	{
		child->ForEachRecursivePostOrder(func);
	}

	func(this);  // Apply to self last
}

void Struktur::UI::UIElement::Dispose(GameContext& context)
{
	if (m_onClickCallback)
	{
		m_onClickCallback.Dispose(context);
	}
	if (m_onFocusCallback)
	{
		m_onFocusCallback.Dispose(context);
	}
	if (m_onLoseFocusCallback)
	{
		m_onLoseFocusCallback.Dispose(context);
	}
	if (m_onHoverCallback)
	{
		m_onHoverCallback.Dispose(context);
	}
	if (m_onActivateCallback)
	{
		m_onActivateCallback.Dispose(context);
	}
	if (m_onKeyPressedCallback)
	{
		m_onKeyPressedCallback.Dispose(context);
	}

	// Dispose children recursively
	for (auto& child : m_children)
	{
		child->Dispose(context);
	}

	// Batch-root elements own their batch's bgfx buffers (see AssignBatches) - without this, destroying the
	// widget would leak them, and worse, leave the batch itself active in UIRenderer's m_batches, still being
	// drawn by Flush() forever with whatever stale vertex/texture data it last held.
	if (m_isBatchRoot && m_ownBatch.IsValid())
	{
		context.GetUIRenderer().DestroyBatch(m_ownBatch);
		m_ownBatch = Renderer::UIBatchHandle{};
	}

	m_disposed = true;
}

void Struktur::UI::UIElement::OnClick(GameContext& context, const glm::vec2& mousePos)
{
	if (m_onClickCallback)
	{
		m_onClickCallback.Invoke(context, this, mousePos);
	}
}

void Struktur::UI::UIElement::OnHover(GameContext& context, const glm::vec2& mousePos)
{
	if (m_onHoverCallback)
	{
		m_onHoverCallback.Invoke(context, this, mousePos);
	}
}

void Struktur::UI::UIElement::OnFocus(GameContext& context)
{
	if (m_onFocusCallback)
	{
		m_onFocusCallback.Invoke(context, this);
	}
}

void Struktur::UI::UIElement::OnLoseFocus(GameContext& context)
{
	if (m_onLoseFocusCallback)
	{
		m_onLoseFocusCallback.Invoke(context, this);
	}
}

void Struktur::UI::UIElement::OnButtonPressed(GameContext& context, int key)
{
	if (m_onKeyPressedCallback)
	{
		m_onKeyPressedCallback.Invoke(context, this, key);
	}
}

void Struktur::UI::UIElement::OnActivate(GameContext& context)
{
	if (m_onActivateCallback)
	{
		m_onActivateCallback.Invoke(context, this);
	}
	else
	{
		// Default behavior - trigger click
		OnClick(context, GetPosition());
	}
}

void Struktur::UI::UIElement::RenderFocusIndicator(GameContext& context)
{
	// Simple focus indicator - can be customized
	context.GetUIRenderer().DrawRectOutline(m_bounds, 2.0f, Util::ColorBlue);
}

void Struktur::UI::UIElement::UpdateChildren(GameContext& context)
{
	for (auto& child : m_children)
	{
		if (child->IsVisible() && child->IsEnabled())
		{
			child->Update(context);
		}
	}
}

void Struktur::UI::UIElement::RenderChildren(GameContext& context)
{
	// No z-index sort here - children write into stable, pre-allocated batch slots (see AssignBatches), so the
	// order Render() is called in doesn't control final draw order. Quads sharing a batch draw in z-index order
	// via UIBatch::quadZIndex/sortedQuadOrder (see UIRenderer::WriteRect/Flush) regardless of call order.
	for (auto& child : m_children)
	{
		if (child->IsVisible())
		{
			child->Render(context);
		}
	}
}

void Struktur::UI::UIElement::UpdateBounds()
{
	glm::vec2 position = GetPosition();
	glm::vec2 size     = GetSize();
	m_bounds           = Util::Math::Rect{position.x, position.y, size.x, size.y};
}
