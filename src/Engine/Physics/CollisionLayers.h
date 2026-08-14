#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

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
	// Allocates the next free bit for `name` if it isn't already registered, otherwise returns its existing bit.
	// Returns 0 (no bit assigned) if all 16 bits are already in use.
	uint16_t RegisterLayer(const std::string& name);
	// Returns the bit previously allocated to `name` via RegisterLayer, or 0 if `name` was never registered.
	uint16_t GetLayer(const std::string& name) const;
	// Reverse lookup - the name registered for `bit` (must be a single-bit value, not a combined mask), or an
	// empty string if nothing is registered there.
	const std::string& GetLayerName(uint16_t bit) const;

	bool HasLayer(const std::string& name) const;

   private:
	std::unordered_map<std::string, uint16_t> m_layers;
	std::unordered_map<uint16_t, std::string> m_layerNames;
	uint16_t m_nextBit = 0x0001;
};
}  // namespace Physics
}  // namespace Struktur
