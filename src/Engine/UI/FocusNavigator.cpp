#include "FocusNavigator.h"

#include "Engine/GameContext.h"

void Struktur::UI::FocusNavigator::Update(GameContext& context)
{
	float deltaTime = context.GetTimeSystem().unscaledDelta;
	m_currentCooldownTimer -= deltaTime;
}

void Struktur::UI::FocusNavigator::RegisterElement(UIElement* element)
{
	if (element && element->IsFocusable())
	{
		auto it = std::find(m_focusableElements.begin(), m_focusableElements.end(), element);
		if (it == m_focusableElements.end())
		{
			m_focusableElements.push_back(element);
			SortElementsByTabIndex();
		}
	}
}

void Struktur::UI::FocusNavigator::UnregisterElement(UIElement* element)
{
	auto it = std::find(m_focusableElements.begin(), m_focusableElements.end(), element);
	if (it != m_focusableElements.end())
	{
		if (m_currentFocus == element)
		{
			m_currentFocus = nullptr;
		}

		m_focusableElements.erase(it);
	}
}

bool Struktur::UI::FocusNavigator::NavigateDirection(GameContext& context, NavigationDirection direction)
{
	if (m_currentCooldownTimer > 0.0f || !m_currentFocus)
	{
		return false;
	}

	UIElement* nextElement = FindNextElement(direction);
	if (nextElement)
	{
		SetFocus(context, nextElement);
		m_currentCooldownTimer = m_navigationCooldown;
		return true;
	}
	return false;
}

namespace
{
bool IsNavigable(const Struktur::UI::UIElement* element)
{
	return element->IsEffectivelyVisible() && element->IsEnabled() && element->IsFocusable();
}
}  // namespace

bool Struktur::UI::FocusNavigator::NavigateToNext(GameContext& context)
{
	if (m_focusableElements.empty())
	{
		return false;
	}

	size_t startIndex;
	if (!m_currentFocus)
	{
		startIndex = m_focusableElements.size() - 1;  // so step 1 below lands on index 0 first
	}
	else
	{
		auto it = std::find(m_focusableElements.begin(), m_focusableElements.end(), m_currentFocus);
		if (it == m_focusableElements.end())
		{
			return false;
		}
		startIndex = static_cast<size_t>(std::distance(m_focusableElements.begin(), it));
	}

	// Walk forward from the current element, skipping any candidate that isn't actually navigable (see
	// IsNavigable) - bounded by the list's own size so an all-invisible list terminates instead of spinning
	// forever.
	for (size_t step = 1; step <= m_focusableElements.size(); ++step)
	{
		size_t index          = (startIndex + step) % m_focusableElements.size();
		UIElement* candidate = m_focusableElements[index];
		if (IsNavigable(candidate))
		{
			SetFocus(context, candidate);
			return true;
		}
	}
	return false;
}

bool Struktur::UI::FocusNavigator::NavigateToPrevious(GameContext& context)
{
	if (m_focusableElements.empty())
	{
		return false;
	}

	size_t startIndex;
	if (!m_currentFocus)
	{
		startIndex = 0;  // so step 1 below lands on back() first
	}
	else
	{
		auto it = std::find(m_focusableElements.begin(), m_focusableElements.end(), m_currentFocus);
		if (it == m_focusableElements.end())
		{
			return false;
		}
		startIndex = static_cast<size_t>(std::distance(m_focusableElements.begin(), it));
	}

	for (size_t step = 1; step <= m_focusableElements.size(); ++step)
	{
		// + size() before the modulo so this stays well-defined for unsigned arithmetic when startIndex - step
		// would otherwise underflow.
		size_t index          = (startIndex + m_focusableElements.size() - step) % m_focusableElements.size();
		UIElement* candidate = m_focusableElements[index];
		if (IsNavigable(candidate))
		{
			SetFocus(context, candidate);
			return true;
		}
	}
	return false;
}

void Struktur::UI::FocusNavigator::SetFocus(GameContext& context, UIElement* element)
{
	if (m_currentFocus == element)
	{
		return;
	}

	if (m_currentFocus)
	{
		m_currentFocus->OnLoseFocus(context);
	}

	m_currentFocus = element;

	if (m_currentFocus)
	{
		m_currentFocus->OnFocus(context);
	}
}

Struktur::UI::UIElement* Struktur::UI::FocusNavigator::GetCurrentFocus() const
{
	return m_currentFocus;
}

void Struktur::UI::FocusNavigator::Clear(GameContext& context)
{
	if (m_currentFocus)
	{
		m_currentFocus->OnLoseFocus(context);
	}
	m_currentFocus = nullptr;
	for (auto& focusable : m_focusableElements)
	{
		focusable->SetFocusable(false);
	}
	m_focusableElements.clear();
}

Struktur::UI::UIElement* Struktur::UI::FocusNavigator::FindNextElement(NavigationDirection direction)
{
	if (!m_currentFocus)
	{
		return nullptr;
	}

	// First check manual navigation neighbors
	UIElement* neighbor = m_currentFocus->GetNavigationNeighbor(direction);
	if (neighbor && neighbor->IsEffectivelyVisible() && neighbor->IsEnabled() && neighbor->IsFocusable())
	{
		return neighbor;
	}

	// Fall back to spatial navigation
	return FindElementByDirection(direction);
}

Struktur::UI::UIElement* Struktur::UI::FocusNavigator::FindElementByDirection(NavigationDirection direction)
{
	if (!m_currentFocus)
	{
		return nullptr;
	}

	glm::vec2 currentPos = m_currentFocus->GetPosition();
	UIElement* bestMatch = nullptr;
	float bestDistance   = FLT_MAX;

	for (UIElement* element : m_focusableElements)
	{
		if (element == m_currentFocus || !element->IsEffectivelyVisible() || !element->IsEnabled() ||
		    !element->IsFocusable())
		{
			continue;
		}

		glm::vec2 elementPos = element->GetPosition();
		bool isInDirection   = false;
		float distance       = 0.0f;

		switch (direction)
		{
			case NavigationDirection::UP:
				isInDirection = elementPos.y < currentPos.y;
				distance      = currentPos.y - elementPos.y + abs(currentPos.x - elementPos.x) * 0.5f;
				break;
			case NavigationDirection::DOWN:
				isInDirection = elementPos.y > currentPos.y;
				distance      = elementPos.y - currentPos.y + abs(currentPos.x - elementPos.x) * 0.5f;
				break;
			case NavigationDirection::LEFT:
				isInDirection = elementPos.x < currentPos.x;
				distance      = currentPos.x - elementPos.x + abs(currentPos.y - elementPos.y) * 0.5f;
				break;
			case NavigationDirection::RIGHT:
				isInDirection = elementPos.x > currentPos.x;
				distance      = elementPos.x - currentPos.x + abs(currentPos.y - elementPos.y) * 0.5f;
				break;
		}

		if (isInDirection && distance < bestDistance)
		{
			bestDistance = distance;
			bestMatch    = element;
		}
	}

	return bestMatch;
}

void Struktur::UI::FocusNavigator::SortElementsByTabIndex()
{
	std::sort(m_focusableElements.begin(), m_focusableElements.end(),
	          [](UIElement* a, UIElement* b)
	          {
		          int aTab = a->GetTabIndex();
		          int bTab = b->GetTabIndex();

		          // Elements with no tab index go to the end
		          if (aTab == -1 && bTab == -1)
		          {
			          return false;
		          }
		          if (aTab == -1)
		          {
			          return false;
		          }
		          if (bTab == -1)
		          {
			          return true;
		          }

		          return aTab < bTab;
	          });
}
