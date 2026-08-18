#include "WrenUI.h"

#ifdef DEBUG
	#include <Trace/wren_trace.h>
#endif
#include "Engine/Callback/WrenCallback.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/UI/UIBorder.h"
#include "Engine/UI/UIColor.h"
#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIPanel.h"
#include "Engine/UI/UIRichLabel.h"
#include "Engine/UI/UITexture.h"
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

	// First batch-assignment pass for this tree. Marking the root as a batch root (rather than just handing
	// AssignBatches a batch created here) means AssignBatches creates and records it as m_ownBatch itself - which
	// is what makes UIElement::Dispose() actually destroy it later. Without this, removing the tree via
	// UIManager.removeUIElement would leak its GPU buffers and, worse, leave the batch active in UIRenderer's
	// m_batches, still being drawn by Flush() forever with whatever it last held.
	uiElement->element->SetBatchRoot(true);
	context->GetUIRenderer().AssignBatches(uiElement->element, Struktur::Renderer::UIBatchHandle{});
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

	// SetZIndex alone only reorders quads within a shared batch (see UIElement::SetZIndex's own comment) - a
	// batch root's cross-batch order (UIBatch::drawOrder) needs this explicit follow-up, the same "renderer-side
	// state needs its own sync call" contract SetBatchRoot's comment already documents for AssignBatches.
	if (uiElement->element->IsBatchRoot())
	{
		Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
		context->GetUIRenderer().SetBatchDrawOrder(uiElement->element->GetBatch(), zIndex);
	}
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

	// Re-run assignment for the whole tree (not just uiElement->element) - assignment always walks from the
	// root (see UIRenderer::AssignBatches), and the new child can shift which batch it and its own subtree
	// should share. Reuses the root's already-assigned ambient batch (set by UIManager.addUIElement's initial
	// pass, or an earlier one of these calls) if it has one, rather than minting a new one every time.
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIElement* root                  = uiElement->element->GetRoot();
	Struktur::Renderer::UIBatchHandle ambientBatch = root->GetBatch();
	if (!ambientBatch.IsValid())
	{
		ambientBatch = context->GetUIRenderer().CreateBatch();
	}
	context->GetUIRenderer().AssignBatches(root, ambientBatch);
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
	WrenUIElement* child = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 1));
	bool success         = uiElement->element->RemoveChild(child->element);
	wrenSetSlotBool(vm, 0, success);

	// Same re-assignment as addChild above - only needed if a child actually came out.
	if (success)
	{
		Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
		Struktur::UI::UIElement* root                  = uiElement->element->GetRoot();
		Struktur::Renderer::UIBatchHandle ambientBatch = root->GetBatch();
		if (!ambientBatch.IsValid())
		{
			ambientBatch = context->GetUIRenderer().CreateBatch();
		}
		context->GetUIRenderer().AssignBatches(root, ambientBatch);
	}
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
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
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

// UIPanel.setBackgroundColor(color)
void wren_UIPanelSetBackgroundColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIPanel.setBackgroundColor: panel is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
	Struktur::UI::UIPanel* panel = dynamic_cast<Struktur::UI::UIPanel*>(uiElement->element);
	panel->SetBackgroundColor(rayColor);
}

// UIPanel.setBorderColor(color)
void wren_UIPanelSetBorderColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIPanel.setBorderColor: panel is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
	Struktur::UI::UIPanel* panel = dynamic_cast<Struktur::UI::UIPanel*>(uiElement->element);
	panel->SetBorderColor(rayColor);
}

// UIPanel.setBorderWidth(width)
void wren_UIPanelSetBorderWidth(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIPanel.setBorderWidth: panel is Null");
		return;
	}
	float width                   = (float)wrenGetSlotDouble(vm, 1);
	Struktur::UI::UIPanel* panel = dynamic_cast<Struktur::UI::UIPanel*>(uiElement->element);
	panel->SetBorderWidth(width);
}

// ============================================================================
// UICOLOR BINDINGS
// ============================================================================

void wren_UIColorAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

void wren_UIColorFinalize(void* data)
{
	WrenUIElement* uiElement = (WrenUIElement*)data;
	uiElement->~WrenUIElement();
}

// UIColor.new(_,_,_,_)
void wren_UIColorNew(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));

	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	WrenVec2* sizePixel          = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* sizePercentage     = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	auto* color = new Struktur::UI::UIColor(positionPixel->value, positionPercentage->value, sizePixel->value,
	                                        sizePercentage->value);
	new (uiElement) WrenUIElement{color};
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UIColor.setColor(color)
void wren_UIColorSetColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIColor.setColor: element is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
	Struktur::UI::UIColor* uiColor = dynamic_cast<Struktur::UI::UIColor*>(uiElement->element);
	uiColor->SetColor(rayColor);
}

// ============================================================================
// UITEXTURE BINDINGS
// ============================================================================

void wren_UITextureAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

void wren_UITextureFinalize(void* data)
{
	WrenUIElement* uiElement = (WrenUIElement*)data;
	uiElement->~WrenUIElement();
}

// UITexture.new(_,_,_,_)
void wren_UITextureNew(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));

	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	WrenVec2* sizePixel          = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* sizePercentage     = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	auto* texture = new Struktur::UI::UITexture(positionPixel->value, positionPercentage->value, sizePixel->value,
	                                            sizePercentage->value);
	new (uiElement) WrenUIElement{texture};
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UITexture.setTexture(texture)
void wren_UITextureSetTexture(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UITexture.setTexture: element is Null");
		return;
	}
	WrenTextureHandle* texture         = (WrenTextureHandle*)wrenGetSlotForeign(vm, 1);
	Struktur::UI::UITexture* uiTexture = dynamic_cast<Struktur::UI::UITexture*>(uiElement->element);
	uiTexture->SetTexture(texture->resource);
}

// UITexture.setTint(color)
void wren_UITextureSetTint(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UITexture.setTint: element is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
	Struktur::UI::UITexture* uiTexture = dynamic_cast<Struktur::UI::UITexture*>(uiElement->element);
	uiTexture->SetTint(rayColor);
}

// ============================================================================
// UIBORDER BINDINGS
// ============================================================================

void wren_UIBorderAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

void wren_UIBorderFinalize(void* data)
{
	WrenUIElement* uiElement = (WrenUIElement*)data;
	uiElement->~WrenUIElement();
}

// UIBorder.new(_,_,_,_)
void wren_UIBorderNew(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));

	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	WrenVec2* sizePixel          = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* sizePercentage     = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	auto* border = new Struktur::UI::UIBorder(positionPixel->value, positionPercentage->value, sizePixel->value,
	                                          sizePercentage->value);
	new (uiElement) WrenUIElement{border};
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UIBorder.setColor(color)
void wren_UIBorderSetColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIBorder.setColor: element is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
	Struktur::UI::UIBorder* uiBorder = dynamic_cast<Struktur::UI::UIBorder*>(uiElement->element);
	uiBorder->SetColor(rayColor);
}

// UIBorder.setWidth(width)
void wren_UIBorderSetWidth(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIBorder.setWidth: element is Null");
		return;
	}
	float width                     = (float)wrenGetSlotDouble(vm, 1);
	Struktur::UI::UIBorder* uiBorder = dynamic_cast<Struktur::UI::UIBorder*>(uiElement->element);
	uiBorder->SetWidth(width);
}

// ============================================================================
// ICONATLAS BINDINGS - not a UIElement (see WrenIconAtlasHandle), just a data value UIRichLabel.setIconAtlas
// copies out of.
// ============================================================================

void wren_IconAtlasAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenIconAtlasHandle));
}

void wren_IconAtlasFinalize(void* data)
{
	WrenIconAtlasHandle* handle = static_cast<WrenIconAtlasHandle*>(data);
	handle->~WrenIconAtlasHandle();
}

// IconAtlas.new()
void wren_IconAtlasNew(WrenVM* vm)
{
	WrenIconAtlasHandle* handle = static_cast<WrenIconAtlasHandle*>(wrenGetSlotForeign(vm, 0));
	new (handle) WrenIconAtlasHandle();
}

// IconAtlas.setTexture(texture)
void wren_IconAtlasSetTexture(WrenVM* vm)
{
	WrenIconAtlasHandle* handle = static_cast<WrenIconAtlasHandle*>(wrenGetSlotForeign(vm, 0));
	WrenTextureHandle* texture  = static_cast<WrenTextureHandle*>(wrenGetSlotForeign(vm, 1));
	handle->atlas.SetTexture(texture->resource);
}

// IconAtlas.addIcon(name, x, y, width, height)
void wren_IconAtlasAddIcon(WrenVM* vm)
{
	WrenIconAtlasHandle* handle = static_cast<WrenIconAtlasHandle*>(wrenGetSlotForeign(vm, 0));
	const char* name            = wrenGetSlotString(vm, 1);
	float x                     = (float)wrenGetSlotDouble(vm, 2);
	float y                     = (float)wrenGetSlotDouble(vm, 3);
	float width                 = (float)wrenGetSlotDouble(vm, 4);
	float height                = (float)wrenGetSlotDouble(vm, 5);
	handle->atlas.AddIcon(name, Struktur::Util::Math::Rect{x, y, width, height});
}

// ============================================================================
// UIRICHLABEL BINDINGS
// ============================================================================

void wren_UIRichLabelAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

void wren_UIRichLabelFinalize(void* data)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(data);
	uiElement->~WrenUIElement();
}

// UIRichLabel.new(_,_,_) / new(_,_,_,_)
void wren_UIRichLabelNew(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenUIElement* uiElement       = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	WrenVec2* positionPixel        = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage   = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	const char* markupText         = wrenGetSlotString(vm, 3);
	if (wrenGetSlotCount(vm) >= 5)
	{
		float fontSize = static_cast<float>(wrenGetSlotDouble(vm, 4));
		auto* richLabel = new Struktur::UI::UIRichLabel(*context, positionPixel->value, positionPercentage->value,
		                                                markupText, fontSize);
		new (uiElement) WrenUIElement{richLabel};
	}
	else
	{
		auto* richLabel =
		    new Struktur::UI::UIRichLabel(*context, positionPixel->value, positionPercentage->value, markupText);
		new (uiElement) WrenUIElement{richLabel};
	}
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UIRichLabel.setFont(font)
void wren_UIRichLabelSetFont(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setFont: element is Null");
		return;
	}
	WrenFontHandle* font                  = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetFont(font->resource);
}

// UIRichLabel.setBoldFont(font)
void wren_UIRichLabelSetBoldFont(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setBoldFont: element is Null");
		return;
	}
	WrenFontHandle* font                  = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetBoldFont(font->resource);
}

// UIRichLabel.setItalicFont(font)
void wren_UIRichLabelSetItalicFont(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setItalicFont: element is Null");
		return;
	}
	WrenFontHandle* font                  = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetItalicFont(font->resource);
}

// UIRichLabel.setBoldItalicFont(font)
void wren_UIRichLabelSetBoldItalicFont(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setBoldItalicFont: element is Null");
		return;
	}
	WrenFontHandle* font                  = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetBoldItalicFont(font->resource);
}

// UIRichLabel.setTextColor(color)
void wren_UIRichLabelSetTextColor(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setTextColor: element is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetTextColor(rayColor);
}

// UIRichLabel.setFontSize(size)
void wren_UIRichLabelSetFontSize(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setFontSize: element is Null");
		return;
	}
	float size                            = static_cast<float>(wrenGetSlotDouble(vm, 1));
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetFontSize(size);
}

// UIRichLabel.setWordWrap(wordWrap)
void wren_UIRichLabelSetWordWrap(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setWordWrap: element is Null");
		return;
	}
	Struktur::UI::TextWrapping textWrapping = static_cast<Struktur::UI::TextWrapping>(wrenGetSlotDouble(vm, 1));
	Struktur::UI::UIRichLabel* richLabel    = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetWordWrap(textWrapping);
}

// UIRichLabel.setMarkupText(newMarkupText)
void wren_UIRichLabelSetMarkupText(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setMarkupText: element is Null");
		return;
	}
	const char* newMarkupText             = wrenGetSlotString(vm, 1);
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetMarkupText(newMarkupText);
}

// UIRichLabel.getMarkupText()
void wren_UIRichLabelGetMarkupText(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.getMarkupText: element is Null");
		return;
	}
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	wrenSetSlotString(vm, 0, richLabel->GetMarkupText().c_str());
}

// UIRichLabel.setIconAtlas(atlas)
void wren_UIRichLabelSetIconAtlas(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setIconAtlas: element is Null");
		return;
	}
	WrenIconAtlasHandle* atlasHandle      = static_cast<WrenIconAtlasHandle*>(wrenGetSlotForeign(vm, 1));
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetIconAtlas(atlasHandle->atlas);
}

// UIRichLabel.setVisibleGlyphCount(count)
void wren_UIRichLabelSetVisibleGlyphCount(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.setVisibleGlyphCount: element is Null");
		return;
	}
	int count                             = (int)wrenGetSlotDouble(vm, 1);
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	richLabel->SetVisibleGlyphCount(count);
}

// UIRichLabel.getGlyphCount() - total drawable glyph count (see GetRequiredQuadCount), for a Wren-side reveal
// loop to compare its own elapsed-time-driven count against (matching Godot's own
// visible_characters/text-length pattern for a typewriter effect).
void wren_UIRichLabelGetGlyphCount(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (!uiElement->element)
	{
		DEBUG_ERROR("UIRichLabel.getGlyphCount: element is Null");
		return;
	}
	Struktur::UI::UIRichLabel* richLabel = dynamic_cast<Struktur::UI::UIRichLabel*>(uiElement->element);
	wrenSetSlotDouble(vm, 0, (double)richLabel->GetRequiredQuadCount());
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
	WREN_CLASS_METHOD(registry, "ui", "UIPanel", "setBackgroundColor(_)", wren_UIPanelSetBackgroundColor,
	                  "Sets the UI Panels background color");
	WREN_CLASS_METHOD(registry, "ui", "UIPanel", "setBorderColor(_)", wren_UIPanelSetBorderColor,
	                  "Sets the UI Panels border color");
	WREN_CLASS_METHOD(registry, "ui", "UIPanel", "setBorderWidth(_)", wren_UIPanelSetBorderWidth,
	                  "Sets the UI Panels border width");

	// Register UIColor foreign class - minimal solid-color rect (see UIColor.h), for when you only want a color
	// fill without UIPanel's bundled texture/border support.
	WREN_FOREIGN_CLASS(registry, "ui", "UIColor", wren_UIColorAllocate, wren_UIColorFinalize,
	                   "UI minimal solid-color rect component");
	WREN_CLASS_INHERITANCE(registry, "ui", "UIColor", "UIElement");
	WREN_CONSTRUCTOR(registry, "ui", "UIColor", "new(_,_,_,_)", wren_UIColorNew,
	                 "Create UIColor with absolutePosition, relativePosition, absoluteSize, relativeSize components");
	WREN_CLASS_METHOD(registry, "ui", "UIColor", "setColor(_)", wren_UIColorSetColor, "Sets the UIColor's color");

	// Register UITexture foreign class - minimal textured rect (see UITexture.h), for when you only want a
	// texture without UIPanel's bundled solid-color/border support.
	WREN_FOREIGN_CLASS(registry, "ui", "UITexture", wren_UITextureAllocate, wren_UITextureFinalize,
	                   "UI minimal textured rect component");
	WREN_CLASS_INHERITANCE(registry, "ui", "UITexture", "UIElement");
	WREN_CONSTRUCTOR(
	    registry, "ui", "UITexture", "new(_,_,_,_)", wren_UITextureNew,
	    "Create UITexture with absolutePosition, relativePosition, absoluteSize, relativeSize components");
	WREN_CLASS_METHOD(registry, "ui", "UITexture", "setTexture(_)", wren_UITextureSetTexture,
	                  "Sets the UITexture's texture");
	WREN_CLASS_METHOD(registry, "ui", "UITexture", "setTint(_)", wren_UITextureSetTint,
	                  "Sets the UITexture's tint color");

	// Register UIBorder foreign class - minimal border outline (see UIBorder.h), for when you only want a border
	// without UIPanel's bundled solid-color/texture support.
	WREN_FOREIGN_CLASS(registry, "ui", "UIBorder", wren_UIBorderAllocate, wren_UIBorderFinalize,
	                   "UI minimal border outline component");
	WREN_CLASS_INHERITANCE(registry, "ui", "UIBorder", "UIElement");
	WREN_CONSTRUCTOR(registry, "ui", "UIBorder", "new(_,_,_,_)", wren_UIBorderNew,
	                 "Create UIBorder with absolutePosition, relativePosition, absoluteSize, relativeSize components");
	WREN_CLASS_METHOD(registry, "ui", "UIBorder", "setColor(_)", wren_UIBorderSetColor, "Sets the UIBorder's color");
	WREN_CLASS_METHOD(registry, "ui", "UIBorder", "setWidth(_)", wren_UIBorderSetWidth, "Sets the UIBorder's width");

	// Register IconAtlas foreign class - not a UIElement, just a name->rect data table over a texture (see
	// IconAtlas.h) that UIRichLabel.setIconAtlas copies out of.
	WREN_FOREIGN_CLASS(registry, "ui", "IconAtlas", wren_IconAtlasAllocate, wren_IconAtlasFinalize,
	                   "Named-icon lookup table over a texture, for UIRichLabel's [icon=name] tags");
	WREN_CONSTRUCTOR(registry, "ui", "IconAtlas", "new()", wren_IconAtlasNew, "Create an empty IconAtlas");
	WREN_CLASS_METHOD(registry, "ui", "IconAtlas", "setTexture(_)", wren_IconAtlasSetTexture,
	                  "Sets the texture this atlas's icons are sub-rects of");
	WREN_CLASS_METHOD(registry, "ui", "IconAtlas", "addIcon(_,_,_,_,_)", wren_IconAtlasAddIcon,
	                  "Registers name -> a pixel-space (x, y, width, height) sub-rect of the atlas texture");

	// Register UIRichLabel foreign class - markup-driven text ([b]/[i]/[color=#rrggbb]/[icon=name], see
	// Text::ParseMarkup), a sibling to UILabel rather than a subclass of it (see UIRichLabel.h's class
	// comment for why).
	WREN_FOREIGN_CLASS(registry, "ui", "UIRichLabel", wren_UIRichLabelAllocate, wren_UIRichLabelFinalize,
	                   "Markup-driven UI Label component supporting inline bold/italic/color/icon formatting");
	WREN_CLASS_INHERITANCE(registry, "ui", "UIRichLabel", "UIElement");
	WREN_CONSTRUCTOR(registry, "ui", "UIRichLabel", "new(_,_,_)", wren_UIRichLabelNew,
	                 "Create UIRichLabel with absolutePosition, relativePosition, markupText components");
	WREN_CONSTRUCTOR(registry, "ui", "UIRichLabel", "new(_,_,_,_)", wren_UIRichLabelNew,
	                 "Create UIRichLabel with absolutePosition, relativePosition, markupText, fontSize components");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setFont(_)", wren_UIRichLabelSetFont,
	                  "Sets the UIRichLabel's regular-style font");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setBoldFont(_)", wren_UIRichLabelSetBoldFont,
	                  "Sets the font used for [b] runs");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setItalicFont(_)", wren_UIRichLabelSetItalicFont,
	                  "Sets the font used for [i] runs");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setBoldItalicFont(_)", wren_UIRichLabelSetBoldItalicFont,
	                  "Sets the font used for [b][i] runs");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setTextColor(_)", wren_UIRichLabelSetTextColor,
	                  "Sets the UIRichLabel's base text color (the color runs without an explicit [color=] use)");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setFontSize(_)", wren_UIRichLabelSetFontSize,
	                  "Sets the UIRichLabel's font size");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setWordWrap(_)", wren_UIRichLabelSetWordWrap,
	                  "Sets the UIRichLabel's word wrap");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setMarkupText(_)", wren_UIRichLabelSetMarkupText,
	                  "Sets the UIRichLabel's markup text");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "getMarkupText()", wren_UIRichLabelGetMarkupText,
	                  "Gets the UIRichLabel's markup text");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setIconAtlas(_)", wren_UIRichLabelSetIconAtlas,
	                  "Sets the IconAtlas [icon=name] tags resolve against");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "setVisibleGlyphCount(_)", wren_UIRichLabelSetVisibleGlyphCount,
	                  "Caps rendering to the first N drawable glyphs, for a typewriter/reveal effect - negative "
	                  "means show everything");
	WREN_CLASS_METHOD(registry, "ui", "UIRichLabel", "getGlyphCount()", wren_UIRichLabelGetGlyphCount,
	                  "Gets the total drawable glyph count, for a reveal loop to compare its progress against");
}
