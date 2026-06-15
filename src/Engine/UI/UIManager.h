#pragma once

#include "Engine/UI/FocusNavigator.h"
#include "Engine/UI/UIElement.h"
#include "raylib.h"

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
   public:
	UIManager();

	~UIManager() = default;

	UIElement* AddElement(std::unique_ptr<UIElement> element);

	template <typename T, typename... Args>
	T* CreateElement(Args&&... args)
	{
		auto element = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr       = element.get();
		AddElement(std::move(element));
		return ptr;
	}

	void RemoveElement(GameContext& context, UIElement* element);

	void Update(GameContext& context);
	void Render(GameContext& context);

	void SetFocus(UIElement* element);

	UIElement* GetElementAt(const glm::vec2& position) const;

	void Clear(GameContext& context);

	FocusNavigator* GetFocusNavigator() const;
	const std::vector<std::unique_ptr<UIElement>>& GetElements() const;

   private:
	void HandleInput(GameContext& context);

	std::vector<std::unique_ptr<UIElement>> m_elements;
	UIElement* m_focusedElement;
	bool m_focusJustChanged;
	UIElement* m_hoveredElement;
	bool m_hoveredJustChanged;
	::Camera2D m_camera;
	bool m_capturingInput;
	std::unique_ptr<FocusNavigator> m_focusNavigator;
};
}  // namespace UI
}  // namespace Struktur
