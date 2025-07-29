#include "UIElement.h"

Struktur::UI::UIElement::UIElement(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : m_absolutePosition(absolutePosition), m_relativePosition(relativePosition), 
    m_absoluteSize(absoluteSize), m_relativeSize(relativeSize), m_anchorPoint(glm::vec2()),
    m_backgroundColor(LIGHTGRAY), m_borderColor(DARKGRAY),
    m_borderWidth(1.0f), m_visible(true), m_enabled(true), m_focusable(false),
    m_parent(nullptr), m_zIndex(0), m_tabIndex(-1)
{
    UpdateBounds();
}

Struktur::UI::UIElement* Struktur::UI::UIElement::AddChild(std::unique_ptr<UIElement> child)
{
    if (child)
    {
        child->m_parent = this;
        child->UpdateBounds();
        UIElement* childPtr = child.get();
        m_children.push_back(std::move(child));
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
        return child;
    }
    return nullptr;
}

bool Struktur::UI::UIElement::RemoveChild(UIElement* child)
{
	auto it = std::find_if(m_children.begin(), m_children.end(),
		[child](const std::unique_ptr<UIElement>& ptr) {
			return ptr.get() == child;
		});

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
}

void Struktur::UI::UIElement::SetSize(const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
{
    m_absoluteSize = absoluteSize;
    m_relativeSize = relativeSize;
    UpdateBounds();
}

void Struktur::UI::UIElement::SetAnchorPoint(const glm::vec2& anchorPoint)
{
    m_anchorPoint = anchorPoint;
    UpdateBounds();
}

glm::vec2 Struktur::UI::UIElement::GetPosition() const
{
    glm::vec2 basePosition = glm::vec2();
    glm::vec2 baseSize = glm::vec2();
    if (m_parent)
    {
        basePosition = m_parent->GetPosition();
        baseSize = m_parent->GetSize();
    }

    glm::vec2 absolutePosition = m_absolutePosition;
    glm::vec2 relativePosition = baseSize * m_relativePosition;
    glm::vec2 anchorOffset = m_anchorPoint * GetSize();

    return basePosition + absolutePosition + relativePosition - anchorOffset;
}

glm::vec2 Struktur::UI::UIElement::GetSize() const
{
    glm::vec2 basePosition = glm::vec2();
    glm::vec2 baseSize = glm::vec2();
    if (m_parent)
    {
        basePosition = m_parent->GetPosition();
        baseSize = m_parent->GetSize();
    }

    glm::vec2 absoluteSize = m_absoluteSize;
    glm::vec2 relativeSize = baseSize * m_relativeSize;

    return absoluteSize + relativeSize;
}

::Rectangle Struktur::UI::UIElement::GetBounds() const
{
    return m_bounds;
}

bool Struktur::UI::UIElement::IsPointInside(const glm::vec2 &point) const
{
    return CheckCollisionPointRec(Vector2{point.x, point.y}, m_bounds);
}

void Struktur::UI::UIElement::SetNavigationNeighbor(NavigationDirection dir, UIElement *neighbor)
{
    m_navigationNeighbors[static_cast<int>(dir)].clear();
    if (neighbor)
    {
        m_navigationNeighbors[static_cast<int>(dir)].push_back(neighbor);
    }
}

Struktur::UI::UIElement *Struktur::UI::UIElement::GetNavigationNeighbor(NavigationDirection dir) const
{
    int dirIndex = static_cast<int>(dir);
    if (!m_navigationNeighbors[dirIndex].empty())
    {
        return m_navigationNeighbors[dirIndex][0];
    }
    return nullptr;
}

void Struktur::UI::UIElement::RenderFocusIndicator()
{
    // Simple focus indicator - can be customized
    ::DrawRectangleLinesEx(m_bounds, 2.0f, BLUE);
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
    // Sort children by z-index before drawing
    std::vector<UIElement*> sortedChildren;
    for (auto& child : m_children)
    {
        if (child->IsVisible())
        {
            sortedChildren.push_back(child.get());
        }
    }

    std::sort(sortedChildren.begin(), sortedChildren.end(),
        [](UIElement* a, UIElement* b) {
            return a->GetZIndex() < b->GetZIndex();
        });

    for (UIElement* child : sortedChildren)
    {
        child->Render(context);
    }
}

void Struktur::UI::UIElement::UpdateBounds()
{
    glm::vec2 position = GetPosition();
    glm::vec2 size = GetSize();
    m_bounds = ::Rectangle{ position.x, position.y, size.x, size.y };
}
