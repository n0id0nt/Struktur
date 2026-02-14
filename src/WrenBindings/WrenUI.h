#pragma once

#include <vector>
#include "wren.hpp"

#include "Engine/UI/UIElement.h"
#include "Debug/Assertions.h"

struct WrenUIElement
{
	Struktur::UI::UIElement* element;
	bool ownedByWren = true; // ownership of the uiElement must be moved into the UI system but during declaration it isn't so this is a safty check to avoid memoryleaks.

	~WrenUIElement()
	{
		if (ownedByWren) 
		{
			BREAK_MSG("Item is not handed to the UI Manager. This can cause issues with unreleased handles");
			delete element;
		}
	}
};
