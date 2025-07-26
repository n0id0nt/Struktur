#pragma once

#include "raylib.h"

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
            FocusNavigator() : m_currentFocus(nullptr), m_navigationCooldown(0.2f), m_currentCooldownTimer(0.0f) {}

            void Update(GameContext& context);

            void RegisterElement(UIElement* element);
            void UnregisterElement(UIElement* element);

            bool NavigateDirection(NavigationDirection direction);
            bool NavigateToNext();
            bool NavigateToPrevious();

            void SetFocus(UIElement* element);
            UIElement* GetCurrentFocus() const;

            void Clear();

        private:
            UIElement* FindNextElement(NavigationDirection direction);
            UIElement* FindElementByDirection(NavigationDirection direction);

            void SortElementsByTabIndex();
        };
    }
}