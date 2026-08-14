#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Struktur
{
namespace Physics
{
// Name <-> bitmask registry for Box2D collision filtering (b2Filter::categoryBits/maskBits are 16-bit, so up to
// 16 distinct layers - see third_party/box2d/include/box2d/b2_fixture.h). Layers are allocated on first
// RegisterLayer("name") call - idempotent, calling it again with the same name returns the same bit rather than
// allocating a second one. Names are typically registered from Wren at startup (see
// WrenBindings/WrenCollisionLayers.cpp), then referenced by name whenever fixtures are created/filtered (see
// PhysicsSystem::SetCollisionFilter).
class CollisionLayers
{
   public:
	// A mask that matches every layer, registered or not - Box2D's own default maskBits (see b2Filter's
	// constructor), exposed here so callers don't need to know that detail to ask for "collide with everything".
	static constexpr uint16_t kAllLayersMask = 0xFFFF;

	// Allocates the next free bit for `name` if it isn't already registered, otherwise returns its existing bit.
	// Returns 0 (no bit assigned) if all 16 bits are already in use.
	uint16_t RegisterLayer(const std::string& name);
	// Returns the bit previously allocated to `name` via RegisterLayer, or 0 if `name` was never registered.
	uint16_t GetLayer(const std::string& name) const;
	// Reverse lookup - the name registered for `bit` (must be a single-bit value, not a combined mask), or an
	// empty string if nothing is registered there.
	const std::string& GetLayerName(uint16_t bit) const;

	bool HasLayer(const std::string& name) const;

	// Defines (or redefines) `groupName` as the OR of every layer in `layerNames`, auto-registering any name in
	// the list that isn't already a registered layer (so a group's members don't need to be pre-registered
	// separately). Unlike RegisterLayer, groups don't consume one of the 16 category bits - they're just a named
	// alias for a combined mask, stored separately from individual layers, so re-registering a group with a
	// different member list simply replaces its stored mask.
	uint16_t RegisterGroup(const std::string& groupName, const std::vector<std::string>& layerNames);
	// Returns the mask previously defined for `groupName` via RegisterGroup, or 0 if it was never registered.
	uint16_t GetGroup(const std::string& groupName) const;

	bool HasGroup(const std::string& groupName) const;

   private:
	std::unordered_map<std::string, uint16_t> m_layers;
	std::unordered_map<uint16_t, std::string> m_layerNames;
	uint16_t m_nextBit = 0x0001;

	std::unordered_map<std::string, uint16_t> m_groups;
};
}  // namespace Physics
}  // namespace Struktur
