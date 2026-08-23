#pragma once

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace Component
{
struct ParticleEmitter;
}

namespace System
{
// Spawns and simulates Component::ParticleEmitter's particle pool each Update() tick - a plain CPU update
// (age/kill/kinematics), no rendering here. Registered as an update system (see Game.cpp's
// AddUpdateSystem<ParticleSystem>()), which always runs before any AddRenderSystem, so particle state is
// current by the time System::SpriteRenderSystem submits alive particles as sprites later in the same frame
// (see its own comment for why rendering particles lives there instead of here).
class ParticleSystem : public ISystem
{
public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "Particle System";
	}
};
}  // namespace System
}  // namespace Struktur
