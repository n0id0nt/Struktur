#pragma once

#include "raylib.h"

#include "Engine/UI/UIElement.h"
#include "Engine/UI/FocusNavigator.h"

namespace Struktur
{
    class GameContext;

	namespace UI
	{
        //=============================================================================
        // UIManager - Main UI system manager
        //=============================================================================
        class UIManager
        {
        private:
            std::vector<std::unique_ptr<UIElement>> m_elements;
            UIElement* m_focusedElement;
            UIElement* m_hoveredElement;
            ::Camera2D m_camera;
            bool m_capturingInput;
            std::unique_ptr<FocusNavigator> m_focusNavigator;

        public:
            UIManager();

            ~UIManager() = default;

            UIElement* AddElement(std::unique_ptr<UIElement> element);

            template<typename T, typename... Args>
            T* CreateElement(Args&&... args)
            {
                auto element = std::make_unique<T>(std::forward<Args>(args)...);
                T* ptr = element.get();
                AddElement(std::move(element));
                return ptr;
            }

            void RemoveElement(UIElement* element);

            void Update(GameContext& context);
            void Render(GameContext& context);

            void SetFocus(UIElement* element);

            UIElement* GetElementAt(const glm::vec2& position) const;

            void Clear();

            FocusNavigator* GetFocusNavigator() const;

        private:

            void HandleInput(GameContext& context);
        };
    }
}