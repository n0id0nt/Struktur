#pragma once

#include <vector>

#include "Debug/Assertions.h"
#include "Engine/UI/IconAtlas.h"
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

// Not a UIElement - a plain value wrapper around UI::IconAtlas (itself just a name->rect table over a
// TextureResource, see IconAtlas.h), for UIRichLabel.setIconAtlas. No ownership handshake with UIManager is
// needed here (unlike WrenUIElement above), since an IconAtlas is never added to the UI tree itself - it's
// just data a rich label copies out of via setIconAtlas, so the Wren GC can free this immediately once the
// script drops its reference.
struct WrenIconAtlasHandle
{
	Struktur::UI::IconAtlas atlas;
};
