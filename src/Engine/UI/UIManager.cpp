#include "UIManager.h"

#include "Engine/GameContext.h"

Struktur::UI::UIManager::UIManager()
    : m_focusedElement(nullptr),
      m_hoveredElement(nullptr),
      m_capturingInput(false),
      m_focusJustChanged(false),
      m_hoveredJustChanged(false)
{
	m_focusNavigator = std::make_unique<FocusNavigator>();
}

Struktur::UI::UIElement* Struktur::UI::UIManager::AddElement(std::unique_ptr<UIElement> element)
{
	if (element)
	{
		UIElement* ptr = element.get();
		m_elements.push_back(std::move(element));

		// Full-tree attach, not just the root - the tree is almost always built (AddChild'd together) before its
		// root is ever handed here, so this is what actually picks up every already-focusable descendant (see
		// UIElement::AttachToManager's own comment). Mirrors RemoveElement's own recursive teardown walk below.
		ptr->ForEachRecursive([this](UIElement* elem) { elem->AttachToManager(this); });

		return ptr;
	}
	return nullptr;
}

void Struktur::UI::UIManager::RemoveElement(GameContext& context, UIElement* element)
{
	if (!element)
	{
		return;
	}

	// Dispose recurses into every child itself and, per-node, calls OnElementDisposed (below) - that single
	// walk now covers what used to be a separate ForEachRecursive pass here just for focus/hover bookkeeping.
	element->Dispose(context);

	std::erase_if(m_elements, [element](const std::unique_ptr<UIElement>& ptr) { return ptr.get() == element; });
}

void Struktur::UI::UIManager::OnElementDisposed(UIElement* element)
{
	if (m_focusedElement == element)
	{
		m_focusedElement = nullptr;
	}
	if (m_hoveredElement == element)
	{
		m_hoveredElement = nullptr;
	}
	m_focusNavigator->UnregisterElement(element);
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
		m_focusNavigator->SetFocus(context, m_focusedElement);
		m_focusJustChanged = false;
	}
}

Struktur::UI::UIElement* Struktur::UI::UIManager::GetElementAt(const glm::vec2& position) const
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

void Struktur::UI::UIManager::Clear(GameContext& context)
{
	m_focusNavigator->Clear(context);
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
	// UIRenderSystem already set UIViewId's own screen-space ortho projection for this frame (see
	// UIRenderer::SetupView).

	// No z-index sort here - every element in m_elements is a batch root with its own batch (see
	// UIManager::AddElement/wren_UIManagerAddUIElement), and cross-batch draw order is UIBatch::drawOrder's job
	// (set from this element's z-index - see UIRenderer::AssignBatches/SetBatchDrawOrder), not this call order.
	for (auto& element : m_elements)
	{
		if (element->IsVisible())
		{
			element->Render(context);
		}
	}
}

Struktur::UI::FocusNavigator* Struktur::UI::UIManager::GetFocusNavigator() const
{
	return m_focusNavigator.get();
}

const std::vector<std::unique_ptr<Struktur::UI::UIElement>>& Struktur::UI::UIManager::GetElements() const
{
	return m_elements;
}

void Struktur::UI::UIManager::SetFocus(UIElement* element)
{
	m_focusedElement   = element;
	m_focusJustChanged = true;
}

void Struktur::UI::UIManager::HandleInput(GameContext& context)
{
	Input::Input& input = context.GetInput();

	// Handle pointer hover - Input::GetPointerPosition() is one unified coordinate regardless of whether it's
	// currently driven by the real mouse or an active touch (see Input::GetPointerPosition's own comment), so
	// this needs no separate touch-handling branch here.
	UIElement* elementUnderPointer = GetElementAt(input.GetPointerPosition());
	if (elementUnderPointer != m_hoveredElement)
	{
		m_hoveredElement = elementUnderPointer;
		if (m_hoveredElement)
		{
			m_hoveredElement->OnHover(context, input.GetPointerPosition());
		}
	}

	// Handle pointer click - focuses AND activates the hovered element right here, in one motion, rather than
	// relying on the "Handle activation" block below: UIAccept's bindings include the pointer's primary button
	// (see InputConfig.json/Input::RegisterDefaultUIBindings) precisely so a click activates the same way
	// Space/Enter/GamepadA does, but that block only knows about *focus*, not hover - it has no way to tell a
	// legitimate click on the focused element apart from a click anywhere else on screen (empty space, or a
	// completely different, unfocused button) releasing the same physical mouse button. Activating here,
	// gated by m_hoveredElement (bounds-checked against the actual pointer position via GetElementAt above),
	// is the only place that distinction is actually known. `pointerJustReleased` below tells the "Handle
	// activation" block to skip UIAccept entirely for this frame's pointer release, whether or not anything was
	// hovered - a click on empty space must not fall through and activate whatever unrelated element still has
	// focus.
	bool pointerJustReleased = input.IsPointerButtonJustReleased(Input::Input::PointerButton::Primary);
	if (pointerJustReleased && m_hoveredElement && m_hoveredElement->IsFocusable())
	{
		m_focusedElement = m_hoveredElement;
		m_focusNavigator->SetFocus(context, m_hoveredElement);
		m_hoveredElement->OnActivate(context);
	}

	// Handle keyboard navigation
	// float tabAxis = input.GetInputAxis("UITab");
	// if (tabAxis == 0.0f)
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
		if (m_focusNavigator->NavigateDirection(context, navigationDirection))
		{
			SetFocus(m_focusNavigator->GetCurrentFocus());
		}
	}

	// Handle activation - keyboard/gamepad only. A pointer-driven UIAccept release was already handled above,
	// gated by hover; skip it here too (pointerJustReleased, regardless of whether anything was hovered) so a
	// click that lands on empty space - or on a different, unfocused element - can't fall through and activate
	// whatever unrelated element still happens to have focus.
	UIElement* currentFocus = m_focusNavigator->GetCurrentFocus();
	if (currentFocus && !pointerJustReleased)
	{
		bool accept = input.IsInputJustReleased("UIAccept");
		if (accept)
		{
			currentFocus->OnActivate(context);
		}
		// if (keyboard.lastKeyPressed != 0)
		//{
		//     currentFocus->OnKeyPressed(keyboard.lastKeyPressed);
		// }
	}
}
