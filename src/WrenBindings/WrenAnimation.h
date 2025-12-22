#include "Engine/Animation/SpriteAnimation.h"

struct WrenSpriteAnimationDefinition
{
	Struktur::Animation::SpriteAnimation spriteAnimation;

	WrenSpriteAnimationDefinition() : spriteAnimation() {}
	WrenSpriteAnimationDefinition(const Struktur::Animation::SpriteAnimation& spriteAnimation) : spriteAnimation(spriteAnimation) {}
};