#include <string>
#include <vector>

#include "wren.hpp"

#include "Engine/GameContext.h"
#include "Engine/Physics/CollisionLayers.h"
#include "Engine/Scripting/WrenBindingRegistry.h"

// ============================================================================
// COLLISION LAYERS BINDINGS - name <-> bitmask registry for Box2D collision filtering
// ============================================================================

// CollisionLayers.registerLayer(name)
void wren_CollisionLayersRegisterLayer(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	const char* name = wrenGetSlotString(vm, 1);
	uint16_t bit      = collisionLayers.RegisterLayer(name);
	wrenSetSlotDouble(vm, 0, static_cast<double>(bit));
}

// CollisionLayers.getLayer(name)
void wren_CollisionLayersGetLayer(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	const char* name = wrenGetSlotString(vm, 1);
	uint16_t bit      = collisionLayers.GetLayer(name);
	wrenSetSlotDouble(vm, 0, static_cast<double>(bit));
}

// CollisionLayers.getLayerName(bit)
void wren_CollisionLayersGetLayerName(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	uint16_t bit             = static_cast<uint16_t>(wrenGetSlotDouble(vm, 1));
	const std::string& name = collisionLayers.GetLayerName(bit);
	wrenSetSlotString(vm, 0, name.c_str());
}

// CollisionLayers.hasLayer(name)
void wren_CollisionLayersHasLayer(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	const char* name = wrenGetSlotString(vm, 1);
	bool exists      = collisionLayers.HasLayer(name);
	wrenSetSlotBool(vm, 0, exists);
}

// CollisionLayers.registerGroup(groupName, layerNames)
void wren_CollisionLayersRegisterGroup(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	const char* groupName = wrenGetSlotString(vm, 1);

	if (wrenGetSlotType(vm, 2) != WREN_TYPE_LIST)
	{
		wrenSetSlotString(vm, 0, "Expected a list of layer names");
		wrenAbortFiber(vm, 0);
		return;
	}

	// One slot beyond the two arguments (self is slot 0, groupName is slot 1, the list itself is slot 2) - needed
	// as scratch space to pull each list element out via wrenGetListElement below.
	wrenEnsureSlots(vm, 4);

	int count = wrenGetListCount(vm, 2);
	std::vector<std::string> layerNames;
	layerNames.reserve(count);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 2, i, 3);
		layerNames.push_back(wrenGetSlotString(vm, 3));
	}

	uint16_t mask = collisionLayers.RegisterGroup(groupName, layerNames);
	wrenSetSlotDouble(vm, 0, static_cast<double>(mask));
}

// CollisionLayers.getGroup(groupName)
void wren_CollisionLayersGetGroup(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	const char* groupName = wrenGetSlotString(vm, 1);
	uint16_t mask          = collisionLayers.GetGroup(groupName);
	wrenSetSlotDouble(vm, 0, static_cast<double>(mask));
}

// CollisionLayers.hasGroup(groupName)
void wren_CollisionLayersHasGroup(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Physics::CollisionLayers& collisionLayers = context->GetCollisionLayers();

	const char* groupName = wrenGetSlotString(vm, 1);
	bool exists            = collisionLayers.HasGroup(groupName);
	wrenSetSlotBool(vm, 0, exists);
}

WREN_BINDING_MODULE(CollisionLayers)
{
	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "registerLayer(_)", wren_CollisionLayersRegisterLayer,
	                  "Allocates a bit for the given layer name if it isn't already registered, and returns it "
	                  "(0 if all 16 bits are already in use). Calling again with the same name returns the same "
	                  "bit.");
	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "getLayer(_)", wren_CollisionLayersGetLayer,
	                  "Returns the bit previously registered for this layer name via registerLayer, or 0 if it "
	                  "was never registered.");
	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "getLayerName(_)", wren_CollisionLayersGetLayerName,
	                  "Returns the layer name registered for this bit, or an empty string if nothing is "
	                  "registered there.");
	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "hasLayer(_)", wren_CollisionLayersHasLayer,
	                  "Returns true if this layer name has already been registered.");

	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "registerGroup(_,_)",
	                  wren_CollisionLayersRegisterGroup,
	                  "Defines (or redefines) a named group as the combined mask of the given list of layer "
	                  "names, auto-registering any that aren't already registered layers, and returns the "
	                  "combined mask.");
	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "getGroup(_)", wren_CollisionLayersGetGroup,
	                  "Returns the combined mask previously defined for this group name via registerGroup, or 0 "
	                  "if it was never registered.");
	WREN_CLASS_STATIC(registry, "physics", "CollisionLayers", "hasGroup(_)", wren_CollisionLayersHasGroup,
	                  "Returns true if this group name has already been registered.");

	WREN_CLASS_CONSTANT(registry, "physics", "CollisionLayers", ALL, Struktur::Physics::CollisionLayers::kAllLayersMask,
	                    "Mask value that collides with every possible layer, registered or not (Box2D's own "
	                    "default maskBits) - use this instead of a group when you want everything.");
}
