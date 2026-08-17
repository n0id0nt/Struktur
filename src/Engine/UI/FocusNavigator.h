#pragma once

#include "Engine/UI/UIElement.h"

namespace Struktur
{
class GameContext;

namespace UI
{
//=============================================================================
// FocusNavigator - Handles focus management and navigation
//=============================================================================
class FocusNavigator
{
private:
	std::vector<UIElement*> m_focusableElements;
	UIElement* m_currentFocus;
	float m_navigationCooldown;
	float m_currentCooldownTimer;

public:
	FocusNavigator()
	    : m_currentFocus(nullptr),
	      m_navigationCooldown(0.2f),
	      m_currentCooldownTimer(0.0f)
	{
	}

	void Update(GameContext& context);

	void RegisterElement(UIElement* element);
	void UnregisterElement(UIElement* element);

	bool NavigateDirection(GameContext& context, NavigationDirection direction);
	bool NavigateToNext(GameContext& context);
	bool NavigateToPrevious(GameContext& context);

	void SetFocus(GameContext& context, UIElement* element);
	UIElement* GetCurrentFocus() const;

	void Clear(GameContext& context);

private:
	UIElement* FindNextElement(NavigationDirection direction);
	UIElement* FindElementByDirection(NavigationDirection direction);

	void SortElementsByTabIndex();
};
}  // namespace UI
}  // namespace Struktur
