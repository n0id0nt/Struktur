#pragma once

#include "Engine/UI/FocusNavigator.h"
#include "Engine/UI/UIElement.h"

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

	// Called by UIElement::Dispose() (once per node, since Dispose already recurses into every child) as the
	// single place a UIElement detaches from everything this manager was tracking about it: clears
	// m_focusedElement/m_hoveredElement if either pointed at it, and unregisters it from the FocusNavigator.
	// Not meant to be called from anywhere else - UIElement is the only thing that knows its own lifecycle is
	// actually ending.
	void OnElementDisposed(UIElement* element);

private:
	void HandleInput(GameContext& context);

	std::vector<std::unique_ptr<UIElement>> m_elements;
	UIElement* m_focusedElement;
	bool m_focusJustChanged;
	UIElement* m_hoveredElement;
	bool m_hoveredJustChanged;
	bool m_capturingInput;
	std::unique_ptr<FocusNavigator> m_focusNavigator;
};
}  // namespace UI
}  // namespace Struktur
