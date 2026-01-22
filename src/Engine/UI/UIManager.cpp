#include "UIManager.h"

#include "Engine/GameContext.h"

Struktur::UI::UIManager::UIManager()
    : m_focusedElement(nullptr), m_hoveredElement(nullptr), m_capturingInput(false), m_focusJustChanged(false), m_hoveredJustChanged(false)
{
    m_camera = {{0, 0}, {0, 0}, 0.0f, 1.0f};
    m_focusNavigator = std::make_unique<FocusNavigator>();
}

Struktur::UI::UIElement *Struktur::UI::UIManager::AddElement(std::unique_ptr<UIElement> element)
{
    if (element)
    {
        UIElement* ptr = element.get();
        m_elements.push_back(std::move(element));
        
        if (ptr->IsFocusable())
        {
            m_focusNavigator->RegisterElement(ptr);
        }
        
        return ptr;
    }
    return nullptr;
}

void Struktur::UI::UIManager::RemoveElement(GameContext& context, UIElement *element)
{
    if (!element) return;
    
    // Process element and all its children recursively
    element->ForEachRecursive([this](UIElement* elem) {
        // Check if this element is focused
        if (m_focusedElement == elem) m_focusedElement = nullptr;
        if (m_hoveredElement == elem) m_hoveredElement = nullptr;
        
        // Remove from focus navigator
        m_focusNavigator->UnregisterElement(elem);
    });
    
    element->Dispose(context);

    std::erase_if(m_elements,
        [element](const std::unique_ptr<UIElement>& ptr) {
            return ptr.get() == element;
        });
}

void Struktur::UI::UIManager::Update(GameContext& context)
{
    m_focusNavigator->Update(context);
    
    // Update all elements
    for (auto& element : m_elements)
    {
        if (element->IsVisible() && element->IsEnabled())
        {
            element->Update(context);
        }
    }
    
    HandleInput(context);
    // Process change in focus
    if (m_focusJustChanged)
    {
        m_focusNavigator->SetFocus(m_focusedElement);
        m_focusJustChanged = false;
    }
}

Struktur::UI::UIElement *Struktur::UI::UIManager::GetElementAt(const glm::vec2 &position) const
{
    // Check elements in reverse z-order (top to bottom)
    for (auto it = m_elements.rbegin(); it != m_elements.rend(); ++it)
    {
        UIElement* element = it->get();
        if (element->IsVisible() && element->IsEnabled() && element->IsPointInside(position))
        {
            return element;
        }
    }
    return nullptr;
}

void Struktur::UI::UIManager::Clear(GameContext &context)
{
    m_focusNavigator->Clear();
    m_focusedElement = nullptr;
    m_hoveredElement = nullptr;
    for (auto& element : m_elements)
    {
        element->Dispose(context);
    }
    m_elements.clear();
}

void Struktur::UI::UIManager::Render(GameContext& context)
{
    ::BeginMode2D(m_camera);
    
    // Sort elements by z-index
    std::vector<UIElement*> sortedElements;
    for (auto& element : m_elements)
    {
        if (element->IsVisible())
        {
            sortedElements.push_back(element.get());
        }
    }
    
    std::sort(sortedElements.begin(), sortedElements.end(),
        [](UIElement* a, UIElement* b) {
            return a->GetZIndex() < b->GetZIndex();
        });
    
    // Draw all elements
    for (UIElement* element : sortedElements)
    {
        element->Render(context);
    }
    
    // Draw focus indicator on top
    if (m_focusedElement)
    {
        m_focusedElement->RenderFocusIndicator();
    }
    
    ::EndMode2D();
}

Struktur::UI::FocusNavigator *Struktur::UI::UIManager::GetFocusNavigator() const
{
    return m_focusNavigator.get();
}

const std::vector<std::unique_ptr<Struktur::UI::UIElement>> &Struktur::UI::UIManager::GetElements() const
{
    return m_elements;
}

void Struktur::UI::UIManager::SetFocus(UIElement* element)
{
    m_focusedElement = element;
    m_focusJustChanged = true;
}

void Struktur::UI::UIManager::HandleInput(GameContext &context)
{
    Core::Input& input = context.GetInput();
    
    // Handle mouse hover
    //if (mouse.hasMoved) {
    //    UIElement* elementUnderMouse = GetElementAt(mouse.position);
    //    if (elementUnderMouse != hoveredElement) {
    //        if (hoveredElement) {
    //            // Mouse left previous element
    //        }
    //        hoveredElement = elementUnderMouse;
    //        if (hoveredElement) {
    //            hoveredElement->OnHover(mouse.position);
    //        }
    //    }
    //}
    
    // Handle mouse clicks
    //if (mouse.leftButton && m_hoveredElement) {
    //    SetFocus(m_hoveredElement);
    //    m_hoveredElement->OnClick(mouse.position);
    //}
    
    // Handle keyboard navigation
    //float tabAxis = input.GetInputAxis("UITab");
    //if (tabAxis == 0.0f)
    //{
    //    if (tabAxis > 0)
    //    {
    //        m_focusNavigator->NavigateToPrevious();
    //    }
    //    else
    //    {
    //        m_focusNavigator->NavigateToNext();
    //    }
    //}
    
    // Handle arrow key navigation
    glm::vec2 inputDir = input.GetInputAxis2("UIDir");
    if (inputDir != glm::vec2())
    {
        NavigationDirection navigationDirection = NavigationDirection::UP;
        if (inputDir.y > 0.0f)
        {
            navigationDirection = NavigationDirection::UP;
        }
        else if (inputDir.y < 0.0f)
        {
            navigationDirection = NavigationDirection::DOWN;
        }
        else if (inputDir.x < 0.0f)
        {
            navigationDirection = NavigationDirection::LEFT;
        }
        else if (inputDir.x > 0.0f)
        {
            navigationDirection = NavigationDirection::RIGHT;
        }
        if (m_focusNavigator->NavigateDirection(navigationDirection))
        {
            SetFocus(m_focusNavigator->GetCurrentFocus());
        }
    }
    
    // Handle activation
    UIElement* currentFocus = m_focusNavigator->GetCurrentFocus();
    if (currentFocus)
    {
        bool accept = input.IsInputJustReleased("UIAccept");
        if (accept)
        {
            currentFocus->OnActivate();
        }
        //if (keyboard.lastKeyPressed != 0)
        //{
        //    currentFocus->OnKeyPressed(keyboard.lastKeyPressed);
        //}
    }
}
