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

// Application.registerComponentScript(module, className)
void wren_ApplicationRegisterComponentScript(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& scriptComponentRegistry = context->GetWrenScriptComponentRegistry();
	const char* module = wrenGetSlotString(vm, 1);
	const char* className = wrenGetSlotString(vm, 2);
    
    scriptComponentRegistry.RegisterScriptComponent(module, className);
}

// Application.deltaTime -> number
void wren_ApplicationGetDeltaTime(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double deltaTime = static_cast<double>(gameData.deltaTime);
	wrenSetSlotDouble(vm, 0, deltaTime);
}

// Application.gameTime -> number
void wren_ApplicationGetGameTime(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double gameTime = static_cast<double>(gameData.gameTime);
	wrenSetSlotDouble(vm, 0, gameTime);
}

// Application.gameWidth -> number
void wren_ApplicationGetGameWidth(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double gameWidth = static_cast<double>(gameData.gameWidth);
	wrenSetSlotDouble(vm, 0, gameWidth);
}

// Application.gameHeight -> number
void wren_ApplicationGetGameHeight(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double gameHeight = static_cast<double>(gameData.gameHeight);
	wrenSetSlotDouble(vm, 0, gameHeight);
}

// Application.pixelsPerMeter -> number
void wren_ApplicationGetPixelsPerMeter(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double pixelsPerMeter = static_cast<double>(gameData.pixelsPerMeter);
	wrenSetSlotDouble(vm, 0, pixelsPerMeter);
}

// Application.setPixelsPerMeter(number)
void wren_ApplicationSetPixelsPerMeter(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double pixelsPerMeter = wrenGetSlotDouble(vm, 1);
	gameData.pixelsPerMeter = static_cast<float>(pixelsPerMeter);
}

// Application.velocityIterations -> number
void wren_ApplicationGetVelocityIterations(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double velocityIterations = static_cast<double>(gameData.velocityIterations);
	wrenSetSlotDouble(vm, 0, velocityIterations);
}

// Application.setVelocityIterations(number)
void wren_ApplicationSetVelocityIterations(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double velocityIterations = wrenGetSlotDouble(vm, 1);
	gameData.velocityIterations = static_cast<int>(velocityIterations);
}

// Application.positionIterations -> number
void wren_ApplicationGetPositionIterations(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double positionIterations = static_cast<double>(gameData.positionIterations);
	wrenSetSlotDouble(vm, 0, positionIterations);
}

// Application.setPositionIterations(number)
void wren_ApplicationSetPositionIterations(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();
	double positionIterations = wrenGetSlotDouble(vm, 1);
	gameData.positionIterations = static_cast<int>(positionIterations);
}

WREN_CLASS_STATIC("app", "Application", "setWindowSize(_,_)", wren_ApplicationSetWindowSize, "Change the size of the game window.");
WREN_CLASS_STATIC("app", "Application", "setApplicationName(_)", wren_ApplicationSetApplicationName, "Changes the name of the game window.");
WREN_CLASS_STATIC("app", "Application", "registerComponentScript(_,_)", wren_ApplicationRegisterComponentScript, "Changes the name of the game window.");
WREN_CLASS_STATIC("app", "Application", "deltaTime", wren_ApplicationGetDeltaTime, "Get the games delta time.");
WREN_CLASS_STATIC("app", "Application", "gameTime", wren_ApplicationGetGameTime, "Get the games time.");
WREN_CLASS_STATIC("app", "Application", "gameWidth", wren_ApplicationGetGameWidth, "Get the games width.");
WREN_CLASS_STATIC("app", "Application", "gameHeight", wren_ApplicationGetGameHeight, "Get the games height.");
WREN_CLASS_STATIC("app", "Application", "pixelsPerMeter", wren_ApplicationGetPixelsPerMeter, "Get the pixels per meter for the physics system.");
WREN_CLASS_STATIC("app", "Application", "setPixelsPerMeter(_)", wren_ApplicationSetPixelsPerMeter, "Set the pixels per meter for the physics system.");
WREN_CLASS_STATIC("app", "Application", "velocityIterations", wren_ApplicationGetVelocityIterations, "Get the velocity iterations for the physics system.");
WREN_CLASS_STATIC("app", "Application", "setVelocityIterations(_)", wren_ApplicationSetVelocityIterations, "Set the velocity iterations for the physics system.");
WREN_CLASS_STATIC("app", "Application", "positionIterations", wren_ApplicationGetPositionIterations, "Get the position iterations for the physics system.");
WREN_CLASS_STATIC("app", "Application", "setPositionIterations(_)", wren_ApplicationSetPositionIterations, "Set the position iterations for the physics system.");

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

// Inventory.getItems() -> items
void wren_InventoryGetItems(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& inventory = context->GetInventory();

	wrenSetSlotNewList(vm, 0);
	int index = 0;
	for (auto& item : inventory)
	{
		wrenSetSlotString(vm, 1, item.c_str());
		wrenInsertInList(vm, 0, index, 1);
		index++;
	}
}

// Inventory.addItem(item) -> bool
void wren_InventoryAddItem(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& inventory = context->GetInventory();

	const char* itemName = wrenGetSlotString(vm, 1);

	if (std::find(inventory.begin(), inventory.end(), itemName) == inventory.end())
	{
		inventory.push_back(itemName);
		wrenSetSlotBool(vm, 0, true);
	}
	else
	{
		wrenSetSlotBool(vm, 0, false);
	}
}

// Inventory.removeItem(item) -> bool
void wren_InventoryRemoveItem(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& inventory = context->GetInventory();

	const char* itemName = wrenGetSlotString(vm, 1);

	if (std::find(inventory.begin(), inventory.end(), itemName) == inventory.end())
	{
		inventory.erase(std::find(inventory.begin(), inventory.end(), itemName));
		wrenSetSlotBool(vm, 0, true);
	}
	else
	{
		wrenSetSlotBool(vm, 0, false);
	}
}

WREN_CLASS_STATIC("app", "Inventory", "contains(_)", wren_InventoryContains, "Check if the inventory contains an item.");
WREN_CLASS_STATIC("app", "Inventory", "getItems()", wren_InventoryGetItems, "Get a list of all items in inventory.");
WREN_CLASS_STATIC("app", "Inventory", "addItem(_)", wren_InventoryAddItem, "add an item to the inventory.");
WREN_CLASS_STATIC("app", "Inventory", "removeItem(_)", wren_InventoryRemoveItem, "remove an item to the inventory.");