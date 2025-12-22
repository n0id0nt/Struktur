#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// APPLICATION BINDINGS
// ============================================================================

// Application.setWindowSize(windowWidth, windowHeight)
void wren_ApplicationSetWindowSize(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();

	double windowWidthDouble = wrenGetSlotDouble(vm, 1);
	double windowHeightDouble = wrenGetSlotDouble(vm, 2);
	int windowWidth = static_cast<int>(windowWidthDouble);
	int windowHeight = static_cast<int>(windowHeightDouble);

	gameData.gameWidth = windowWidth;
	gameData.gameHeight = windowHeight;

	// TODO check if the window size needs to be changed
	// this can be called before there is a window so need to handle that case as well
	// need to handle debug and editor builds when the game and application size does not match
}

// Application.setApplicationName(name)
void wren_ApplicationSetApplicationName(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
    
	const char* name = wrenGetSlotString(vm, 1);
    
	gameData.projectName = name;
    
	// TODO check if the window name needs to be changed here
	// this can be called before there is a window so need to handle that case as well
}

WREN_CLASS_STATIC("app", "Application", "setWindowSize(_,_)", wren_ApplicationSetWindowSize, "Change the size of the game window.");
WREN_CLASS_STATIC("app", "Application", "setApplicationName(_)", wren_ApplicationSetApplicationName, "Changes the name of the game window.");

// ============================================================================
// INVENTORY BINDINGS
// ============================================================================

// Inventory.contains(item) -> bool
void wren_InventoryContains(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& inventory = context->GetInventory();

	const char* itemName = wrenGetSlotString(vm, 1);

	bool containsItem = std::find(inventory.begin(), inventory.end(), itemName) != inventory.end();

	wrenSetSlotBool(vm, 0, containsItem);
}

WREN_CLASS_STATIC("app", "Inventory", "contains(_)", wren_InventoryContains, "Check if an item is contained in the inventorty.");