#pragma once

#include <vector>

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/Util/Color.h"
#include "Engine/World/RenderLayer.h"
#include "glm/glm.hpp"

namespace Struktur
{
namespace Component
{
// One slot in a ParticleEmitter's fixed-capacity pool (see ParticleEmitter::particles) - simulated by
// System::ParticleSystem, rendered by System::SpriteRenderSystem (see its own comment for why rendering
// particles lives there rather than in ParticleSystem itself). Deliberately CPU-simulated, not GPU-shader-
// driven - see the particle system design plan for why: this codebase's existing WorldRenderer::SubmitSprite
// pipeline (cull bounds, sort-key batching, atlas frames, per-material Component::Shader) is reused as-is, and
// CPU simulation keeps the door open for anything that isn't closed-form (collision, drag, wind) without an
// architecture change later.
struct Particle
{
	bool alive = false;
	glm::vec2 position{0.0f, 0.0f};
	glm::vec2 velocity{0.0f, 0.0f};
	float age      = 0.0f;
	float lifetime = 1.0f;
	float rotation      = 0.0f;  // radians
	float rotationSpeed = 0.0f;  // radians/sec
};

// Spawns and (via System::ParticleSystem) simulates a pool of Particle slots at this entity's world position
// (see Component::Transform - required alongside this component, same pairing Component::Sprite already
// needs). Mirrors Component::Sprite's plain-data shape and atlas fields (columns/rows/index-equivalent) so a
// particle can reuse the exact same sprite-sheet frame math System::SpriteRenderSystem already has, rather
// than this component needing its own texture-rect logic.
struct ParticleEmitter
{
	Resource::ResourcePtr<Resource::TextureResource> texture;
	// Atlas dimensions for optional flipbook animation - a particle's frame index is 0 (the sheet's first cell)
	// unless/until per-particle flipbook animation is added; carried here now since it's free and matches
	// Component::Sprite's own columns/rows fields exactly.
	int columns = 1;
	int rows    = 1;

	float emissionRate = 10.0f;  // particles/sec while looping is true
	int burstCount      = 0;      // one-shot particles spawned the first Update() this emitter is alive
	float spawnRadius   = 0.0f;   // 0 = point emitter; >0 = uniform-random offset within a circle of this radius

	glm::vec2 velocityMin{-50.0f, -50.0f};
	glm::vec2 velocityMax{50.0f, 50.0f};
	glm::vec2 acceleration{0.0f, 0.0f};  // e.g. gravity

	float lifetimeMin = 0.5f;
	float lifetimeMax = 1.0f;

	Util::Color startColor = Util::ColorWhite;
	Util::Color endColor   = Util::Color{255, 255, 255, 0};  // default: fade to transparent
	float startScale       = 1.0f;
	float endScale         = 1.0f;

	float rotationSpeedMin = 0.0f;
	float rotationSpeedMax = 0.0f;

	// Additive (BGFX_STATE_BLEND_FUNC(SRC_ALPHA, ONE)) instead of normal alpha blending - order-independent,
	// so it sidesteps needing per-particle depth sorting; the natural default for sparks/fire/glow. Set false
	// for a smoke/dust look that needs real alpha blending.
	bool additive = false;
	// Fully-qualified Struktur::World::RenderLayer, not bare World::RenderLayer - this struct is itself inside
	// Struktur::Component, which some translation units (e.g. InspectorWindow.cpp) also see a forward-declared
	// Struktur::Component::World in (the World *component*, unrelated) - unqualified lookup would resolve to
	// that nearer, wrong, incomplete type instead of the Struktur::World *namespace* this field actually means.
	// Component::Sprite's own `layer` field already qualifies fully for the identical reason.
	Struktur::World::RenderLayer layer = Struktur::World::RenderLayer::Entities;
	float orderInLayer                 = 0.0f;

	int maxParticles = 200;  // pool capacity - see Particle's own comment on why this is fixed-size
	bool looping     = true;  // false = burst-only; ParticleSystem stops spawning once the burst has fired

	// Engine-internal state, not meant to be set directly by scripts - written by System::ParticleSystem.
	std::vector<Particle> particles;
	float spawnAccumulator = 0.0f;
	bool hasBurst          = false;
};
}  // namespace Component
}  // namespace Struktur
