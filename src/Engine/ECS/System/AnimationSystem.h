#pragma once

#include <string>

#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace Animation
{
struct SpriteAnimation;
}

namespace System
{
class AnimationSystem : public ISystem
{
public:
	void Update(GameContext& context) override;

	void AddAnimation(GameContext& context, entt::entity entity, const std::string& animationName,
	                  const Animation::SpriteAnimation& animation);
	void PlayAnimation(GameContext& context, entt::entity entity, const std::string& animationName);
	// Starts (or restarts) the animation as if it had already been playing for normalizedTime (0..1) of its
	// duration - lets a script keep a clip in sync with something else's progress (e.g. a charge bar).
	void PlayAnimationAt(GameContext& context, entt::entity entity, const std::string& animationName,
	                     float normalizedTime);
	bool IsAnimationPlaying(GameContext& context, entt::entity entity, const std::string& animationName);

	std::string Name() const override
	{
		return "Animation System";
	}
};
}  // namespace System
}  // namespace Struktur
