#include "UIElement.h"

Struktur::UI::UIElement::UIElement(const glm::vec2 &pos, const glm::vec2 &sz)
    : m_position(pos), m_size(sz), m_backgroundColor(LIGHTGRAY), m_borderColor(DARKGRAY),
    m_borderWidth(1.0f), m_visible(true), m_enabled(true), m_focusable(false),
    m_parent(nullptr), m_zIndex(0), m_tabIndex(-1)
{
    m_bounds = { pos.x, pos.y, sz.x, sz.y };
}

void Struktur::UI::UIElement::AddChild(std::unique_ptr<UIElement> child)
{
    if (child)
    {
        child->m_parent = this;
        m_children.push_back(std::move(child));
    }
}

Struktur::UI::UIElement *Struktur::UI::UIElement::AddChild(UIElement *child)
{
    if (child)
    {
        child->m_parent = this;
        m_children.push_back(std::unique_ptr<UIElement>(child));
        return child;
    }
    return nullptr;
}

void Struktur::UI::UIElement::RemoveChild(UIElement *child)
{
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [child](const std::unique_ptr<UIElement>& ptr) {
                return ptr.get() == child;
            }),
        m_children.end()
    );
}

void Struktur::UI::UIElement::SetPosition(const glm::vec2 &pos)
{
    m_position = pos;
    m_bounds.x = pos.x;
    m_bounds.y = pos.y;
}

void Struktur::UI::UIElement::SetSize(const glm::vec2 &sz)
{
    m_size = sz;
    m_bounds.width = sz.x;
    m_bounds.height = sz.y;
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
