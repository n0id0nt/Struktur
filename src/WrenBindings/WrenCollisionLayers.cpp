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
}
