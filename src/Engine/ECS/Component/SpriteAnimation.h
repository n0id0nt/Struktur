#pragma once

#include <string>
#include <unordered_map>

#include "Engine/Animation/SpriteAnimation.h"

namespace Struktur
{
	namespace Component
	{
		struct SpriteAnimation
		{
			std::unordered_map<std::string, Animation::SpriteAnimation> animations;

			std::string curAnimation;
			float animationStartTime;
		};
    }
}