#include "ParticleSystem.h"

#include <cmath>
#include <random>

#include "Engine/ECS/Component/ParticleEmitter.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "glm/gtc/constants.hpp"

namespace
{
// Shared across every emitter/particle this system ever spawns - a single well-seeded generator is both
// cheaper and better-distributed than reseeding per spawn (see Util::Noise::PerlinNoise1's own file for the
// cost of the opposite approach, rebuilding a table every call).
std::mt19937& RandomEngine()
{
	static std::mt19937 engine{std::random_device{}()};
	return engine;
}

float RandomRange(float min, float max)
{
	if (min >= max)
	{
		return min;
	}
	std::uniform_real_distribution<float> dist(min, max);
	return dist(RandomEngine());
}

// Uniform-in-disc sample (not uniform-in-square-then-reject) via sqrt-of-radius, standard technique for an
// evenly-distributed circular spawn area rather than one biased toward the center.
glm::vec2 RandomInCircle(float radius)
{
	if (radius <= 0.0f)
	{
		return glm::vec2(0.0f, 0.0f);
	}
	float angle = RandomRange(0.0f, glm::two_pi<float>());
	float r     = radius * std::sqrt(RandomRange(0.0f, 1.0f));
	return glm::vec2(std::cos(angle), std::sin(angle)) * r;
}

void SpawnParticle(Struktur::Component::ParticleEmitter& emitter, const glm::vec2& originPosition)
{
	// Find a dead slot to reuse before growing the pool - keeps the vector at its steady-state size instead of
	// reallocating every spawn once the pool has filled up once.
	Struktur::Component::Particle* slot = nullptr;
	for (auto& particle : emitter.particles)
	{
		if (!particle.alive)
		{
			slot = &particle;
			break;
		}
	}
	if (!slot)
	{
		if ((int)emitter.particles.size() >= emitter.maxParticles)
		{
			return;  // pool full - drop the spawn rather than exceeding maxParticles
		}
		emitter.particles.emplace_back();
		slot = &emitter.particles.back();
	}

	*slot           = Struktur::Component::Particle{};
	slot->alive     = true;
	slot->position  = originPosition + RandomInCircle(emitter.spawnRadius);
	slot->velocity  = glm::vec2(RandomRange(emitter.velocityMin.x, emitter.velocityMax.x),
	                            RandomRange(emitter.velocityMin.y, emitter.velocityMax.y));
	slot->lifetime  = RandomRange(emitter.lifetimeMin, emitter.lifetimeMax);
	slot->rotationSpeed = RandomRange(emitter.rotationSpeedMin, emitter.rotationSpeedMax);
}
}  // namespace

void Struktur::System::ParticleSystem::Update(GameContext& context)
{
	entt::registry& registry          = context.GetRegistry();
	TransformSystem& transformSystem  = context.GetSystemManager().GetSystem<TransformSystem>();
	float deltaTime                   = context.GetTimeSystem().scaledDelta;

	// transform itself is unused below - Component::Transform is only required here to gate this view to
	// entities that actually have one (matching Component::Sprite's own pairing), the actual world position
	// comes from TransformSystem::GetWorldPosition (which walks the parent chain, unlike reading the local
	// Transform component directly).
	auto view = registry.view<Component::ParticleEmitter, Component::Transform>(entt::exclude<Inactive>);
	for (auto [entity, emitter, transform] : view.each())
	{
		glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);
		glm::vec2 origin{worldPosition.x, worldPosition.y};

		// Spawn - continuous emission (fractional accumulator so a low rate still spawns correctly over
		// several frames instead of only ever rounding down to zero) plus a one-shot burst.
		if (emitter.looping)
		{
			emitter.spawnAccumulator += emitter.emissionRate * deltaTime;
			while (emitter.spawnAccumulator >= 1.0f)
			{
				SpawnParticle(emitter, origin);
				emitter.spawnAccumulator -= 1.0f;
			}
		}
		if (!emitter.hasBurst)
		{
			for (int i = 0; i < emitter.burstCount; ++i)
			{
				SpawnParticle(emitter, origin);
			}
			emitter.hasBurst = true;
		}

		// Update - simple Euler integration (not exact closed-form kinematics) so this stays easy to extend
		// with non-constant forces (drag, a sampled wind field, ...) later without changing the integration
		// shape - see the particle system design plan for why CPU simulation was chosen specifically to keep
		// this door open.
		for (Component::Particle& particle : emitter.particles)
		{
			if (!particle.alive)
			{
				continue;
			}

			particle.age += deltaTime;
			if (particle.age >= particle.lifetime)
			{
				particle.alive = false;
				continue;
			}

			particle.velocity += emitter.acceleration * deltaTime;
			particle.position += particle.velocity * deltaTime;
			particle.rotation += particle.rotationSpeed * deltaTime;
		}
	}
}
