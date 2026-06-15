#pragma once

#include <vector>

#include "Debug/Assertions.h"
#include "Engine/UI/UIElement.h"
#include "wren.hpp"

struct WrenUIElement
{
	Struktur::UI::UIElement* element;
	bool ownedByWren = true;  // ownership of the uiElement must be moved into the UI system but during declaration it
	                          // isn't so this is a safty check to avoid memoryleaks.
#ifdef DEBUG
	std::string callstack;  // keep track of when the UI element was created
#endif

	~WrenUIElement()
	{
		if (ownedByWren)
		{
			BREAK_MSG("Item is not handed to the UI Manager. This can cause issues with unreleased handles");
			delete element;
		}
	}
};
