#include "Engine/UI/UIElement.h"

struct WrenUIElement
{
	Struktur::UI::UIElement* element;
	bool ownedByWren = true; // ownership of the uiElement must be moved into the UI system but during declaration it isn't so this is a safty check to avoid memoryleaks.

	~WrenUIElement()
	{
		if (ownedByWren) 
		{
			delete element;
		}
	}
};