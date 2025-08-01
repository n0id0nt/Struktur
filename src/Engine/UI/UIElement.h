#pragma once

#include <vector>
#include <string>
#include <memory>
#include "raylib.h"
#include "glm/glm.hpp"
#include <algorithm>
#include <functional>

namespace Struktur
{
    class GameContext;

	namespace UI
	{
        // Enumerations
        enum class NavigationDirection {
            UP, DOWN, LEFT, RIGHT
        };

        class UIElement;

        // Callback function types
        using UIClickCallback = std::function<void(UIElement* sender, const glm::vec2& mousePos)>;
        using UIFocusCallback = std::function<void(UIElement* sender)>;
        using UIHoverCallback = std::function<void(UIElement* sender, const glm::vec2& mousePos)>;
        using UIActivateCallback = std::function<void(UIElement* sender)>;
        using UIKeyCallback = std::function<void(UIElement* sender, int key)>;

        // Event data structure for more complex events
        struct UIEventData {
            UIElement* sender;
            Vector2 mousePosition;
            std::string stringData;
            float numericValue;
            bool boolValue;

            UIEventData(UIElement* s) : sender(s), mousePosition({ 0,0 }),
                numericValue(0.0f), boolValue(false) {
            }
        };

        using UIEventCallback = std::function<void(const UIEventData& eventData)>;

        //=============================================================================
        // UIElement - Base class for all UI elements
        //=============================================================================
        class UIElement
        {
        protected:
            ::Rectangle m_bounds;
            glm::vec2 m_absolutePosition;
            glm::vec2 m_relativePosition;
            glm::vec2 m_absoluteSize;
            glm::vec2 m_relativeSize;
            glm::vec2 m_anchorPoint;
            ::Color m_backgroundColor;
            ::Color m_borderColor;
            float m_borderWidth;
            bool m_visible;
            bool m_enabled;
            bool m_focusable;
            UIElement* m_parent;
            std::vector<std::unique_ptr<UIElement>> m_children;
            std::string m_id;
            int m_zIndex;
            int m_tabIndex;
            std::vector<UIElement*> m_navigationNeighbors[4]; // UP, DOWN, LEFT, RIGHT

            // Callback functions
            UIClickCallback m_onClickCallback;
            UIFocusCallback m_onFocusCallback;
            UIFocusCallback m_onLoseFocusCallback;
            UIHoverCallback m_onHoverCallback;
            UIActivateCallback m_onActivateCallback;
            UIKeyCallback m_onKeyPressedCallback;
            UIEventCallback m_onEventCallback; // Generic event callback

        public:
            UIElement(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize, const glm::vec2& relativeSize);
            virtual ~UIElement() = default;

            // Delete copy constructor and copy assignment operator
            UIElement(const UIElement&) = delete;
            UIElement& operator=(const UIElement&) = delete;

            // Allow move constructor and move assignment operator
            UIElement(UIElement&&) = default;
            UIElement& operator=(UIElement&&) = default;

            // Pure virtual methods that must be implemented
            virtual void Update(GameContext& context) = 0;
            virtual void Render(GameContext& context) = 0;

            // Virtual methods with default implementations
            virtual void OnClick(const glm::vec2& mousePos);
            virtual void OnHover(const glm::vec2& mousePos);
            virtual void OnFocus();
            virtual void OnLoseFocus();
            virtual void OnButtonPressed(int key);
            virtual void OnActivate();
            virtual void RenderFocusIndicator();

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
            ::Rectangle GetBounds() const;

            // Utility methods
            bool IsPointInside(const glm::vec2& point) const;

            // Visibility and state
            void SetVisible(bool vis) { m_visible = vis; }
            void SetEnabled(bool en) { m_enabled = en; }
            bool IsVisible() const { return m_visible; }
            bool IsEnabled() const { return m_enabled; }
            bool IsFocusable() const { return m_focusable; }

            // Focus and navigation
            void SetFocusable(bool focus) { m_focusable = focus; }
            void SetTabIndex(int index) { m_tabIndex = index; }
            int GetTabIndex() const { return m_tabIndex; }

            void SetNavigationNeighbor(NavigationDirection dir, UIElement* neighbor);
            UIElement* GetNavigationNeighbor(NavigationDirection dir) const;

            // Styling
            void SetBackgroundColor(::Color color) { m_backgroundColor = color; }
            void SetBorderColor(::Color color) { m_borderColor = color; }
            void SetBorderWidth(float width) { m_borderWidth = width; }

            // Hierarchy
            UIElement* GetParent() const { return m_parent; }
            const std::vector<std::unique_ptr<UIElement>>& GetChildren() const { return m_children; }

            // ID management
            void SetId(const std::string& elementId) { m_id = elementId; }
            const std::string& GetId() const { return m_id; }

            // Z-order
            void SetZIndex(int z) { m_zIndex = z; }
            int GetZIndex() const { return m_zIndex; }

            // Callback setters - Fluent interface style
            UIElement* SetOnClick(UIClickCallback callback);
            UIElement* SetOnFocus(UIFocusCallback callback);
            UIElement* SetOnLoseFocus(UIFocusCallback callback);
            UIElement* SetOnHover(UIHoverCallback callback);
            UIElement* SetOnKeyPressed(UIKeyCallback callback);
            UIElement* SetOnActivate(UIActivateCallback callback);
            UIElement* SetOnEvent(UIEventCallback callback);
            // Convenience method to trigger generic events
            void TriggerEvent(const std::string& eventType, float numericValue = 0.0f, bool boolValue = false);

        protected:
            void UpdateChildren(GameContext& context);
            void RenderChildren(GameContext& context);

            void UpdateBounds();
        };
    }
}