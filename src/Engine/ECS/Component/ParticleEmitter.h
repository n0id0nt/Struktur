#pragma once

#include <string>
#include <vector>

#include "Engine/Animation/SpriteAnimation.h"

#include "Engine/Resource/FontResource.h"
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
// driven - see the particle system design plan for why: this codebase's existing SpriteBatchRenderer::SubmitSprite
// pipeline (cull bounds, sort-key batching, atlas frames, per-material Component::Shader) is reused as-is, and
// CPU simulation keeps the door open for anything that isn't closed-form (collision, drag, wind) without an
// architecture change later.
// How each particle plays the emitter's sprite animation (ParticleEmitter::animation). Every mode runs on the
// particle's own age, so the particles of one burst animate independently of each other and of the emitter.
enum class ParticleAnimationMode
{
	None,      // no animation: every particle draws the animation's startFrame
	Once,      // plays startFrame..endFrame over animationTime once, then holds the last frame
	Loop,      // plays startFrame..endFrame every animationTime, round and round
	Lifetime,  // the animation's progress is the particle's life progress (age / lifetime) - animationTime unused
};

struct Particle
{
	bool alive = false;
	glm::vec2 position{0.0f, 0.0f};
	glm::vec2 velocity{0.0f, 0.0f};
	float age      = 0.0f;
	float lifetime = 1.0f;
	float rotation      = 0.0f;  // radians
	float rotationSpeed = 0.0f;  // radians/sec
	// A text particle draws this (centred on its position, in the emitter's font) instead of the texture - set at
	// spawn from ParticleEmitter::pendingTexts, so one emitter's particles can each show different text (damage
	// numbers). Empty = an ordinary sprite particle.
	std::string text;
	// The emitter's start/end colours when this particle spawned - so restyling the emitter for the next burst
	// (another element's hit cloud, a heal number after a damage number) never recolours particles in flight.
	Util::Color startColor = Util::ColorWhite;
	Util::Color endColor   = Util::Color{255, 255, 255, 0};
};

// Spawns and (via System::ParticleSystem) simulates a pool of Particle slots at this entity's world position
// (see Component::Transform - required alongside this component, same pairing Component::Sprite already
// needs). Mirrors Component::Sprite's plain-data shape and atlas fields (columns/rows/index-equivalent) so a
// particle can reuse the exact same sprite-sheet frame math System::SpriteRenderSystem already has, rather
// than this component needing its own texture-rect logic.
struct ParticleEmitter
{
	// Pointer-stable storage: a script's ParticleEmitter handle keeps a raw pointer to this component (see
	// WrenBindings/wrenGameObjectComponents.h) for as long as it likes - a battler holds its hit-cloud and text
	// emitters for the whole fight. entt's default removal swaps the last component into the freed slot, which
	// would leave such a handle writing into a moved-from emitter as soon as any other emitter is destroyed.
	static constexpr auto in_place_delete = true;

	Resource::ResourcePtr<Resource::TextureResource> texture;
	// Atlas dimensions, matching Component::Sprite's own columns/rows fields - cells are numbered left to right,
	// top to bottom, the same as a sprite animation's frames.
	int columns = 1;
	int rows    = 1;
	// The sprite animation each particle plays (the same data SpriteAnimationDefinition / Component::SpriteAnimation
	// use: startFrame, endFrame exclusive, animationTime in seconds, loop) and how it's played - see
	// ParticleAnimationMode. The default is no animation on cell 0. `loop` is only read by SetAnimation's
	// mode-from-definition overload (Wren: setAnimation(def)); the mode decides playback.
	Animation::SpriteAnimation animation{0, 1, 1.0f, false};
	ParticleAnimationMode animationMode = ParticleAnimationMode::None;

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

	// Text particles (Particle::text): drawn in `font` at fontSize world pixels (a pixel font snaps to whole
	// multiples of its own size - Text::GlyphScale), scaled by start/endScale, coloured by start/endColor like any
	// particle, never rotated, with a one-pixel outline in outlineColor (alpha 0 = no outline; its alpha follows the
	// particle's fade).
	Resource::ResourcePtr<Resource::FontResource> font;
	float fontSize           = 9.0f;
	Util::Color outlineColor = Util::Color{19, 27, 27, 255};

	// Spawns queued for the next update, on top of looping/burst emission and without clearing live particles
	// (unlike re-arming the burst): one text particle per pendingTexts entry, pendingSpawns plain ones.
	std::vector<std::string> pendingTexts;
	int pendingSpawns = 0;
	float spawnAccumulator = 0.0f;
	bool hasBurst          = false;
};
}  // namespace Component
}  // namespace Struktur
