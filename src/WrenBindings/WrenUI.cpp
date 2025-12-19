#include "WrenUI.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// UI LABEL - Foreign class wrapping glm::vec2
// ============================================================================

// Allocator - called when UILabel.new(pixelPosition, percentagePosition, labelText, fontSize) is invoked
void wren_UILabelAllocate(WrenVM* vm)
{
	// Allocate foreign object
	wrenGetVariable(vm, "ui", "UILabel", 1);  // Get class into slot 1
	WrenUILabel* vec = (WrenUILabel*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenUILabel));
	new (vec) WrenUILabel{ nullptr };
}

// Finalizer - called when garbage collected
void wren_UILabelFinalize(void* data)
{
	WrenUILabel* vec = (WrenUILabel*)data;
	vec->~WrenUILabel();
}

// UILabel.setVisible(isVisible)
void wren_UILabelSetVisible(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setVisible: label is Null");
		return;
	}
	bool isVisible = wrenGetSlotBool(vm, 1);
	handle->label->SetVisible(isVisible);
}

// UILabel.setFont(font)
void wren_UILabelSetFont(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setFont: label is Null");
		return;
	}
	WrenFontHandle* font = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	handle->label->SetFont(font->resource);
}

// UILabel.setTextColor(color)
void wren_UILabelSetTextColor(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setTextColor: label is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	::Color rayColor{ (unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b, (unsigned char)color->value.a };
	handle->label->SetTextColor(rayColor);
}

// UILabel.setPosition(positionPixel, positionPercentage)
void wren_UILabelSetPosition(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setPosition: label is Null");
		return;
	}
	WrenVec2* positionPixel = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	handle->label->SetPosition(positionPixel->value, positionPercentage->value);
}

// UILabel.setAnchorPoint(anchorPoint)
void wren_UILabelSetAnchorPoint(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setAnchorPoint: label is Null");
		return;
	}
	WrenVec2* anchorPoint = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	handle->label->SetAnchorPoint(anchorPoint->value);
}

// Register Sound foreign class
WREN_FOREIGN_CLASS("ui", "UILabel", wren_UILabelAllocate, wren_UILabelFinalize, "UI Label component");

WREN_CLASS_METHOD("ui", "UILabel", "setVisible(_)", wren_UILabelSetVisible, "Sets Label to be visible");
WREN_CLASS_METHOD("ui", "UILabel", "setFont(_)", wren_UILabelSetFont, "Sets the labels font");
WREN_CLASS_METHOD("ui", "UILabel", "setTextColor(_)", wren_UILabelSetTextColor, "Sets the labels text color");
WREN_CLASS_METHOD("ui", "UILabel", "setPosition(_,_)", wren_UILabelSetPosition, "Sets the labels position");
WREN_CLASS_METHOD("ui", "UILabel", "setAnchorPoint(_)", wren_UILabelSetAnchorPoint, "Sets the labels anchor point");
