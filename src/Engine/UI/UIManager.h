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

private:
	void HandleInput(GameContext& context);
	// Focus ring around whichever element currently has focus - a UIManager-owned batch/slot (RenderFocusIndicator
	// used to be a virtual UIElement method instead, but nothing ever overrode it, and focus is inherently
	// UIManager's own concept - which element is focused right now - not any one element's, so centralizing the
	// batch here removes a per-element indirection that was never actually used polymorphically).
	void RenderFocusIndicator(GameContext& context);

	std::vector<std::unique_ptr<UIElement>> m_elements;
	UIElement* m_focusedElement;
	bool m_focusJustChanged;
	UIElement* m_hoveredElement;
	bool m_hoveredJustChanged;
	bool m_capturingInput;
	std::unique_ptr<FocusNavigator> m_focusNavigator;
	Renderer::UIBatchHandle m_focusIndicatorBatch;
	Renderer::UIBatchSlot m_focusIndicatorSlot;
	// True once the focus ring has real (non-degenerated) geometry written - lets Render() clear it exactly once
	// on the transition to "nothing focused" instead of calling ClearSlotFrom every frame in that steady state.
	bool m_focusIndicatorVisible = false;
};
}  // namespace UI
}  // namespace Struktur
