#pragma once

#include <string>
#include "entt/entt.hpp"

#include "Engine/ECS/SystemManager.h"

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

            void AddAnimation(GameContext& context, entt::entity entity, const std::string& animationName, const Animation::SpriteAnimation& animation);
            void PlayAnimation(GameContext& context, entt::entity entity, const std::string& animationName);
            bool IsAnimationPlaying(GameContext& context, entt::entity entity, const std::string& animationName);
		};
	}
}