#pragma once

#include <vector>
#include "wren.hpp"

#include "Engine/UI/UIElement.h"

struct WrenUIElement
{
	Struktur::UI::UIElement* element;
	bool ownedByWren = true; // ownership of the uiElement must be moved into the UI system but during declaration it isn't so this is a safty check to avoid memoryleaks.

	std::vector<WrenHandle*> handles;
	WrenVM* vm = nullptr;  // Need this to release handles

	~WrenUIElement()
	{
		if (ownedByWren) 
		{
			delete element;
		}
		if (vm)
		{
			for (WrenHandle* handle : handles)
			{
				if (handle)
				{
					wrenReleaseHandle(vm, handle);
				}
			}
		}
	}
};