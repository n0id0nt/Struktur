#include "WrenUI.h"

#ifdef DEBUG
	#include <Trace/wren_trace.h>
#endif
#include "Engine/Callback/WrenCallback.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIPanel.h"
#include "WrenMath.h"
#include "WrenResourceManager.h"

// ============================================================================
// UI MANAGER BINDINGS
// ============================================================================

// UIManager.addUIElement(UIElenent)
void wren_UIManagerAddUIElement(WrenVM* vm)
{
	Struktur::GameContext* context     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIManager& uiManager = context->GetUIManager();

	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 1));

	uiManager.AddElement(std::unique_ptr<Struktur::UI::UIElement>(uiElement->element));
	uiElement->ownedByWren = false;
}

// UIManager.removeUIElement(UIElenent)
void wren_UIManagerRemoveUIElement(WrenVM* vm)
{
	Struktur::GameContext* context     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIManager& uiManager = context->GetUIManager();

	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 1));

	ASSERT_MSG(!uiElement->ownedByWren, "UIElement is registered with the UI Manager.");
	uiManager.RemoveElement(*context, uiElement->element);

	uiElement->element = nullptr;
}

// UIManager.setFocus(UIElenent)
void wren_UIManagerSetFocus(WrenVM* vm)
{
	Struktur::GameContext* context     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIManager& uiManager = context->GetUIManager();

	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 1));

	if (!uiElement->element->IsFocusable())
	{
		BREAK_MSG("UIElement is not focusable can't be focused");
		return;
	}
	uiManager.SetFocus(uiElement->element);
}

// UIManager.clearFocusElements()
void wren_UIManagerClearFocusElements(WrenVM* vm)
{
	Struktur::GameContext* context               = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIManager& uiManager           = context->GetUIManager();
	Struktur::UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();
	uiManager.SetFocus(nullptr);
	focusNavigator->Clear(*context);
}

// ============================================================================
// UI Element - Foreign class wrapping UILabel
// ============================================================================

// Allocator - this is a virtual class but still needs one so create an empty ui element
void wren_UIElementAllocate(WrenVM* vm)
{
	// Allocate foreign object
	WrenUIElement* uiElement = (WrenUIElement*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

// Finalizer - called when garbage collected
void wren_UIElementFinalize(void* data)
{
	WrenUIElement* uiElement = (WrenUIElement*)data;
	uiElement->~WrenUIElement();
}

// UIElement.setVisible(isVisible)
void wren_UIElementSetVisible(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setVisible: element is Null");
		return;
	}
	bool isVisible = wrenGetSlotBool(vm, 1);
	uiElement->element->SetVisible(isVisible);
}

// UIElement.getVisible(isVisible)
void wren_UIElementGetVisible(WrenVM* vm)
{
	WrenUIElement* uiElement = (WrenUIElement*)wrenGetSlotForeign(vm, 0);
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getVisible: element is Null");
		return;
	}
	wrenSetSlotBool(vm, 0, uiElement->element->IsVisible());
}

// UIElement.setEnabled(isEnabled)
void wren_UIElementSetEnabled(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setVisible: element is Null");
		return;
	}
	bool isEnabled = wrenGetSlotBool(vm, 1);
	uiElement->element->SetEnabled(isEnabled);
}

// UIElement.getEnabled()
void wren_UIElementGetEnabled(WrenVM* vm)
{
	WrenUIElement* uiElement = (WrenUIElement*)wrenGetSlotForeign(vm, 0);
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getVisible: element is Null");
		return;
	}
	wrenSetSlotBool(vm, 0, uiElement->element->IsEnabled());
}

// UIElement.setFocusable(isFocusable)
void wren_UIElementSetFocusable(WrenVM* vm)
{
	Struktur::GameContext* context               = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIManager& uiManager           = context->GetUIManager();
	Struktur::UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setVisible: element is Null");
		return;
	}
	bool isFocusable = wrenGetSlotBool(vm, 1);
	uiElement->element->SetFocusable(isFocusable);
	focusNavigator->RegisterElement(uiElement->element);
}

// UIElement.getFocusable()
void wren_UIElementGetFocusable(WrenVM* vm)
{
	WrenUIElement* uiElement = (WrenUIElement*)wrenGetSlotForeign(vm, 0);
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getVisible: element is Null");
		return;
	}
	wrenSetSlotBool(vm, 0, uiElement->element->IsFocusable());
}

// UIElement.setPosition(positionPixel, positionPercentage)
void wren_UIElementSetPosition(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: label is Null");
		return;
	}
	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	uiElement->element->SetPosition(positionPixel->value, positionPercentage->value);
}

// UIElement.getPosition()
void wren_UIElementGetPosition(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(uiElement->element->GetPosition());
}

// UIElement.setAnchorPoint(anchorPoint)
void wren_UIElementSetAnchorPoint(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setAnchorPoint: label is Null");
		return;
	}
	WrenVec2* anchorPoint = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	uiElement->element->SetAnchorPoint(anchorPoint->value);
}

// UIElement.setSize(sizePixel, sizePercentage)
void wren_UIElementSetSize(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setSize: uiElement is Null");
		return;
	}
	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	uiElement->element->SetSize(positionPixel->value, positionPercentage->value);
}

// UIElement.getSize()
void wren_UIElementGetSize(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(uiElement->element->GetSize());
}

// UIElement.getBounds()
void wren_UIElementGetBounds(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.getBounds: uiElement is Null");
		return;
	}
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenRect* rect = (WrenRect*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenRect));
	new (rect) WrenRect(uiElement->element->GetBounds());
}

// UIElement.setTabIndex(tabIndex)
void wren_UIElementSetTabIndex(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTabIndex: element is Null");
		return;
	}
	int tabIndex = static_cast<int>(wrenGetSlotDouble(vm, 1));
	uiElement->element->SetTabIndex(tabIndex);
}

// UIElement.getTabIndex()
void wren_UIElementGetTabIndex(WrenVM* vm)
{
	WrenUIElement* uiElement = (WrenUIElement*)wrenGetSlotForeign(vm, 0);
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getTabIndex: element is Null");
		return;
	}
	wrenSetSlotDouble(vm, 0, uiElement->element->GetTabIndex());
}

// UIElement.setNavigationNeighbor(dir, neighborElement)
void wren_UIElementSetNavigationNeighbor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setNavigationNeighbor: element is Null");
		return;
	}
	Struktur::UI::NavigationDirection dir = static_cast<Struktur::UI::NavigationDirection>(wrenGetSlotDouble(vm, 1));
	WrenUIElement* neighborElement        = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	uiElement->element->SetNavigationNeighbor(dir, neighborElement->element);
}

// UIElement.GetNavigationNeighbor(dir)
void wren_UIElementGetNavigationNeighbor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.GetNavigationNeighbor: element is Null");
		return;
	}
	Struktur::UI::NavigationDirection dir = static_cast<Struktur::UI::NavigationDirection>(wrenGetSlotDouble(vm, 1));
	wrenGetVariable(vm, "ui", "UIElement", 1);  // Get class into slot 1
	WrenUIElement* neighborElement = (WrenUIElement*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenUIElement));
	new (neighborElement) WrenUIElement{uiElement->element->GetNavigationNeighbor(dir)};
}

// UIElement.setBackgroundColor(color)
void wren_UIElementSetBackgroundColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setBackgroundColor: element is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b,
	                 (unsigned char)color->value.a};
	uiElement->element->SetBackgroundColor(rayColor);
}

// UIElement.setBorderColor(color)
void wren_UIElementSetBorderColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setBorderColor: element is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b,
	                 (unsigned char)color->value.a};
	uiElement->element->SetBorderColor(rayColor);
}

// UIElement.setBorderWidth(width)
void wren_UIElementSetBorderWidth(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setBorderWidth: element is Null");
		return;
	}
	int width = static_cast<int>(wrenGetSlotDouble(vm, 1));
	uiElement->element->SetBorderWidth(width);
}

// UIElement.getParent()
void wren_UIElementGetParent(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getParent: element is Null");
		return;
	}
	wrenGetVariable(vm, "ui", "UIElement", 1);  // Get class into slot 1
	WrenUIElement* parentElement = (WrenUIElement*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenUIElement));
	new (parentElement) WrenUIElement{uiElement->element->GetParent()};
}

// UIElement.getChildren()
void wren_UIElementGetChildren(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getChildren: element is Null");
		return;
	}
	// TODO return a list of elements
	// wrenGetVariable(vm, "ui", "UIElement", 1);  // Get class into slot 1
	// WrenUIElement* parentElement = (WrenUIElement*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenUIElement));
	// new (parentElement) WrenUIElement{uiElement->element->GetChildren()[0]};
}

// UIElement.setZIndex(zIndex)
void wren_UIElementSetZIndex(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setZIndex: element is Null");
		return;
	}
	int zIndex = static_cast<int>(wrenGetSlotDouble(vm, 1));
	uiElement->element->SetZIndex(zIndex);
}

// UIElement.getZIndex()
void wren_UIElementGetZIndex(WrenVM* vm)
{
	WrenUIElement* uiElement = (WrenUIElement*)wrenGetSlotForeign(vm, 0);
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.getZIndex: element is Null");
		return;
	}
	wrenSetSlotDouble(vm, 0, uiElement->element->GetZIndex());
}

// UIElement.addChild(child)
void wren_UIElementAddChild(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}
	WrenUIElement* child = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 1));
	uiElement->element->AddChild(child->element);
	child->ownedByWren = false;
}

// UIElement.removeChild(child) -> bool
void wren_UIElementRemoveChild(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}
	WrenUIElement* child = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	bool success         = uiElement->element->RemoveChild(child->element);
	wrenSetSlotBool(vm, 0, success);
}

// UIElement.setOnClick { |sender, mousePos| ... }
void wren_UIElementSetOnClick(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}

	WrenHandle* callback = wrenGetSlotHandle(vm, 1);

	uiElement->element->SetOnClick(
	    Struktur::UI::UIClickCallback(std::make_unique<Struktur::Callback::WrenCallback>(callback)));
}

// UIElement.setOnFocus { |sender| ... }
void wren_UIElementSetOnFocus(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}

	WrenHandle* callback = wrenGetSlotHandle(vm, 1);

	uiElement->element->SetOnFocus(
	    Struktur::UI::UIFocusCallback(std::make_unique<Struktur::Callback::WrenCallback>(callback)));
}

// UIElement.setOnLoseFocus { |sender| ... }
void wren_UIElementSetOnLoseFocus(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}

	WrenHandle* callback = wrenGetSlotHandle(vm, 1);

	uiElement->element->SetOnLoseFocus(
	    Struktur::UI::UIFocusCallback(std::make_unique<Struktur::Callback::WrenCallback>(callback)));
}

// UIElement.setOnHover { |sender, mousePos| ... }
void wren_UIElementSetOnHover(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}

	WrenHandle* callback = wrenGetSlotHandle(vm, 1);

	uiElement->element->SetOnHover(
	    Struktur::UI::UIHoverCallback(std::make_unique<Struktur::Callback::WrenCallback>(callback)));
}

// UIElement.setOnKeyPressed { |sender, key| ... }
void wren_UIElementSetOnKeyPressed(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}

	WrenHandle* callback = wrenGetSlotHandle(vm, 1);

	uiElement->element->SetOnKeyPressed(
	    Struktur::UI::UIKeyCallback(std::make_unique<Struktur::Callback::WrenCallback>(callback)));
}

// UIElement.setOnActivate { |sender| ... }
void wren_UIElementSetOnActivate(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setPosition: uiElement is Null");
		return;
	}

	WrenHandle* callback = wrenGetSlotHandle(vm, 1);

	uiElement->element->SetOnActivate(
	    Struktur::UI::UIActivateCallback(std::make_unique<Struktur::Callback::WrenCallback>(callback)));
}

// ============================================================================
// UILABEL BINDINGS
// ============================================================================

// Allocator - this is a virtual class but still needs one so create an empty ui element
void wren_UILabelAllocate(WrenVM* vm)
{
	// Allocate foreign object
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

// Finalizer - called when garbage collected
void wren_UILabelFinalize(void* data)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(data);
	uiElement->~WrenUIElement();
}

// UILabel.new(_,_,_)
void wren_UILabelNew(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenUIElement* uiElement       = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	// Initialise with constructor parameters if provided
	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	const char* labelText        = wrenGetSlotString(vm, 3);
	if (wrenGetSlotCount(vm) >= 5)
	{
		float fontSize = static_cast<float>(wrenGetSlotDouble(vm, 4));
		auto* label =
		    new Struktur::UI::UILabel(*context, positionPixel->value, positionPercentage->value, labelText, fontSize);
		new (uiElement) WrenUIElement{label};
	}
	else
	{
		auto* label = new Struktur::UI::UILabel(*context, positionPixel->value, positionPercentage->value, labelText);
		new (uiElement) WrenUIElement{label};
	}
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UILabel.setFont(font)
void wren_UILabelSetFont(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UILabel.setFont: label is Null");
		return;
	}
	WrenFontHandle* font         = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	Struktur::UI::UILabel* label = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetFont(font->resource);
}

// UILabel.setTextColor(color)
void wren_UILabelSetTextColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTextColor: label is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b,
	                 (unsigned char)color->value.a};
	Struktur::UI::UILabel* label = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetTextColor(rayColor);
}

// UILabel.setAlignment(textAlignment)
void wren_UILabelSetAlignment(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTextAlignment: label is Null");
		return;
	}
	Struktur::UI::TextAlignment textAlignment = static_cast<Struktur::UI::TextAlignment>(wrenGetSlotDouble(vm, 1));
	Struktur::UI::UILabel* label              = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetAlignment(textAlignment);
}

// UILabel.setFontSize(size)
void wren_UILabelSetFontSize(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTextAlignment: label is Null");
		return;
	}
	float size                   = static_cast<float>(wrenGetSlotDouble(vm, 1));
	Struktur::UI::UILabel* label = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetFontSize(size);
}

// UILabel.setWordWrap(wordWrap)
void wren_UILabelSetWordWrap(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTextAlignment: label is Null");
		return;
	}
	Struktur::UI::TextWrapping textWrapping = static_cast<Struktur::UI::TextWrapping>(wrenGetSlotDouble(vm, 1));
	Struktur::UI::UILabel* label            = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetWordWrap(textWrapping);
}

// UILabel.setText(newText)
void wren_UILabelSetText(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTextAlignment: label is Null");
		return;
	}
	const char* newText          = wrenGetSlotString(vm, 1);
	Struktur::UI::UILabel* label = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetText(newText);
}

// UILabel.getText()
void wren_UILabelGetText(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setTextAlignment: label is Null");
		return;
	}
	Struktur::UI::UILabel* label = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	wrenSetSlotString(vm, 1, label->GetText().c_str());
}

// UILabel.setBoundingBoxToText()
void wren_UILabelSetBoundingBoxToText(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIElement.setBoundingBoxToText: label is Null");
		return;
	}
	Struktur::UI::UILabel* label = dynamic_cast<Struktur::UI::UILabel*>(uiElement->element);
	label->SetBoundingBoxToText();
}

// ============================================================================
// UIPANEL BINDINGS
// ============================================================================

// Allocator - this is a virtual class but still needs one so create an empty ui element
void wren_UIPanelAllocate(WrenVM* vm)
{
	// Allocate foreign object
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

// Finalizer - called when garbage collected
void wren_UIPanelFinalize(void* data)
{
	WrenUIElement* uiElement = (WrenUIElement*)data;
	uiElement->~WrenUIElement();
}

// UIPanel.new(_,_,_)
void wren_UIPanelNew(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenUIElement* uiElement       = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	WrenVec2* sizePixel          = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* sizePercentage     = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	auto* panel = new Struktur::UI::UIPanel(positionPixel->value, positionPercentage->value, sizePixel->value,
	                                        sizePercentage->value);
	new (uiElement) WrenUIElement{panel};
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UIPanel.setBackgroundTexture(texture)
void wren_UIPanelSetBackgroundTexture(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIPanel.SetBackgroundTexture: panel is Null");
		return;
	}
	WrenTextureHandle* texture   = (WrenTextureHandle*)wrenGetSlotForeign(vm, 1);
	Struktur::UI::UIPanel* panel = dynamic_cast<Struktur::UI::UIPanel*>(uiElement->element);
	panel->SetBackgroundTexture(texture->resource);
}

// UIPanel.clearBackgroundTexture(texture)
void wren_UIPanelClearBackgroundTexture(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIPanel.SetBackgroundTexture: panel is Null");
		return;
	}
	Struktur::UI::UIPanel* panel = dynamic_cast<Struktur::UI::UIPanel*>(uiElement->element);
	panel->ClearBackgroundTexture();
}

WREN_BINDING_MODULE(UI)
{
	// NAVIGATION DIRECTION BINDINGS
	WREN_ENUM(registry, "ui", NavigationDirection, "Enum to set the navigation direction for UI Elements",
	          WREN_ENUM_PAIR("UP", Struktur::UI::NavigationDirection::UP),
	          WREN_ENUM_PAIR("DOWN", Struktur::UI::NavigationDirection::DOWN),
	          WREN_ENUM_PAIR("LEFT", Struktur::UI::NavigationDirection::LEFT),
	          WREN_ENUM_PAIR("RIGHT", Struktur::UI::NavigationDirection::RIGHT), );

	// TEXT ALIGNMENT BINDINGS
	WREN_ENUM(registry, "ui", TextAlignment, "Enum to set the text alignment for UI Labels",
	          WREN_ENUM_PAIR("LEFT", Struktur::UI::TextAlignment::LEFT),
	          WREN_ENUM_PAIR("CENTER", Struktur::UI::TextAlignment::CENTER),
	          WREN_ENUM_PAIR("RIGHT", Struktur::UI::TextAlignment::RIGHT),
	          WREN_ENUM_PAIR("JUSTIFY", Struktur::UI::TextAlignment::JUSTIFY), );

	// WORD WRAPPING BINDINGS
	WREN_ENUM(registry, "ui", TextWrapping, "Enum to set the text wrapping for UI Labels",
	          WREN_ENUM_PAIR("NONE", Struktur::UI::TextWrapping::NONE),
	          WREN_ENUM_PAIR("WORD_WRAP", Struktur::UI::TextWrapping::WORD_WRAP),
	          WREN_ENUM_PAIR("CHARACTER_WRAP", Struktur::UI::TextWrapping::CHARACTER_WRAP), );

	WREN_CLASS_STATIC(registry, "ui", "UIManager", "addUIElement(_)", wren_UIManagerAddUIElement,
	                  "Add a UI Element to the UI system.");
	WREN_CLASS_STATIC(registry, "ui", "UIManager", "removeUIElement(_)", wren_UIManagerRemoveUIElement,
	                  "Remove and clean up an element in the UI system.");
	WREN_CLASS_STATIC(registry, "ui", "UIManager", "setFocus(_)", wren_UIManagerSetFocus,
	                  "Will set the focus of the current element.");
	WREN_CLASS_STATIC(registry, "ui", "UIManager", "clearFocusElements()", wren_UIManagerClearFocusElements,
	                  "Will set the focus of the current element.");

	// Register Sound foreign class
	WREN_FOREIGN_CLASS(registry, "ui", "UIElement", wren_UIElementAllocate, wren_UIElementFinalize,
	                   "UI Element component");

	// Register instance methods
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "isVisible=(_)", wren_UIElementSetVisible,
	                  "Sets UI Element to be visible");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setVisible(_)", wren_UIElementSetVisible,
	                  "Sets UI Element to be visible");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "isVisible", wren_UIElementGetVisible,
	                  "Gets UI Element to be visible");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "IsEnabled=(_)", wren_UIElementSetEnabled,
	                  "Sets UI Element to be enabled");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "IsEnabled", wren_UIElementGetEnabled,
	                  "Gets UI Element to be enabled");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "IsFocusable=(_)", wren_UIElementSetFocusable,
	                  "Sets UI Element to be focusable");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setFocusable(_)", wren_UIElementSetFocusable,
	                  "Sets UI Element to be focusable");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "IsFocusable", wren_UIElementGetFocusable,
	                  "Gets UI Element to be focusable");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setPosition(_,_)", wren_UIElementSetPosition,
	                  "Sets the UI Elements position");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getPosition()", wren_UIElementGetPosition,
	                  "Gets the UI Elements position");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setAnchorPoint(_)", wren_UIElementSetAnchorPoint,
	                  "Sets the UI Elements anchor point");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setSize(_,_)", wren_UIElementSetSize,
	                  "Sets the UI Elements anchor point");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getSize()", wren_UIElementGetSize, "Gets the UI Elements size");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getBounds()", wren_UIElementGetBounds,
	                  "Gets the UI Elements bounds");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setTabIndex(_)", wren_UIElementSetTabIndex,
	                  "Sets the UI Elements tab index");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getTabIndex()", wren_UIElementGetTabIndex,
	                  "Gets the UI Elements tab index");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setNavigationNeighbor(_,_)", wren_UIElementSetNavigationNeighbor,
	                  "adds a UI Element as a navigation neighbor");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getNavigationNeighbor(_)", wren_UIElementGetNavigationNeighbor,
	                  "Gets a UI Element navigation neighbor from a direction");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setBackgroundColor(_)", wren_UIElementSetBackgroundColor,
	                  "Sets the UI Elements background color");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setBorderColor(_)", wren_UIElementSetBorderColor,
	                  "Sets the UI Elements border color");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setBorderWidth(_)", wren_UIElementSetBorderWidth,
	                  "Sets the UI Elements border width");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getParent()", wren_UIElementGetParent,
	                  "Gets the UI Elements parent");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getChildren()", wren_UIElementGetChildren,
	                  "Sets the UI Elements children");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setZIndex(_)", wren_UIElementSetZIndex,
	                  "Sets the UI Elements Z index");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "getZIndex()", wren_UIElementGetZIndex,
	                  "Sets the UI Elements Z index");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "addChild(_)", wren_UIElementAddChild,
	                  "Adds a UI Element to elements children");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "removeChild(_)", wren_UIElementRemoveChild,
	                  "Removes a UI Element from the children");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setOnClick(_)", wren_UIElementSetOnClick,
	                  "Sets the UI Elements on click callback");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setOnFocus(_)", wren_UIElementSetOnFocus,
	                  "Sets the UI Elements on focus callback");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setOnLoseFocus(_)", wren_UIElementSetOnLoseFocus,
	                  "Sets the UI Elements on lose focus callback");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setOnHover(_)", wren_UIElementSetOnHover,
	                  "Sets the UI Elements on hover callback");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setOnKeyPressed(_)", wren_UIElementSetOnKeyPressed,
	                  "Sets the UI Elements on key pressed callback");
	WREN_CLASS_METHOD(registry, "ui", "UIElement", "setOnActivate(_)", wren_UIElementSetOnActivate,
	                  "Sets the UI Elements on activate callback");
	// WREN_CLASS_METHOD("ui", "UIElement", "setOnEvent(_)", wren_UIElementSetOnEvent, "Sets the UI Elements on event
	// callback");

	// Register Quat foreign class
	WREN_FOREIGN_CLASS(registry, "ui", "UILabel", wren_UILabelAllocate, wren_UILabelFinalize, "UI Label component");
	WREN_CLASS_INHERITANCE(registry, "ui", "UILabel", "UIElement");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "ui", "UILabel", "new(_,_,_)", wren_UILabelNew,
	                 "Create UI Label with absolutePosition, relativePosition, labelText components");
	WREN_CONSTRUCTOR(registry, "ui", "UILabel", "new(_,_,_,_)", wren_UILabelNew,
	                 "Create UI Label with absolutePosition, relativePosition, labelText, fontSize components");

	// Register instance methods
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setFont(_)", wren_UILabelSetFont, "Sets the UI Labels Font");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setTextColor(_)", wren_UILabelSetTextColor,
	                  "Sets the UI Labels text color");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setAlignment(_)", wren_UILabelSetAlignment,
	                  "Sets the UI Labels alignment");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setFontSize(_)", wren_UILabelSetFontSize,
	                  "Sets the UI Labels font size");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setWordWrap(_)", wren_UILabelSetWordWrap,
	                  "Sets the UI Labels word wrap");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setText(_)", wren_UILabelSetText, "set the UI Labels text");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "getText()", wren_UILabelGetText, "get the UI Labels text");
	WREN_CLASS_METHOD(registry, "ui", "UILabel", "setBoundingBoxToText()", wren_UILabelSetBoundingBoxToText,
	                  "Will change the width and height to match the text");

	// Register Quat foreign class
	WREN_FOREIGN_CLASS(registry, "ui", "UIPanel", wren_UIPanelAllocate, wren_UIPanelFinalize, "UI Panel component");
	WREN_CLASS_INHERITANCE(registry, "ui", "UIPanel", "UIElement");

	// Register constructors
	WREN_CONSTRUCTOR(registry, "ui", "UIPanel", "new(_,_,_,_)", wren_UIPanelNew,
	                 "Create UI panel with absolutePosition, relativePosition, absoluteSize, relativeSize components");

	// Register instance methods
	WREN_CLASS_METHOD(registry, "ui", "UIPanel", "setBackgroundTexture(_)", wren_UIPanelSetBackgroundTexture,
	                  "Sets the UI Panels background texture");
	WREN_CLASS_METHOD(registry, "ui", "UIPanel", "ClearBackgroundTexture()", wren_UIPanelClearBackgroundTexture,
	                  "Clears the UI Panels background texture");
}
