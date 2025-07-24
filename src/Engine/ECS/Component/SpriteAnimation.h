#pragma once

#include <string>
#include <unordered_map>

namespace Struktur
{
    namespace Animation
    {
		struct SpriteAnimation
		{
			unsigned int startFrame;
			unsigned int endFrame;
			float animationTime;
			bool loop;
		};
    }

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