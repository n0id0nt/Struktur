#include "WrenAnimation.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// SPRITE ANIMATION DEFINITION BINDINGS
// ============================================================================

// Allocator
void wren_SpriteAnimationDefinitionAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "animtaion", "SpriteAnimationDefinition", 0);  // Get class into slot 1
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenSpriteAnimationDefinition));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 3)
	{
		unsigned int startFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
		unsigned int endFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 2));
		float animationTime = static_cast<float>(wrenGetSlotDouble(vm, 3));
		bool loop = wrenGetSlotBool(vm, 4);
		new (spriteAnimation) WrenSpriteAnimationDefinition({ startFrame, endFrame, animationTime, loop });
	}
	else
	{
		new (spriteAnimation) WrenSpriteAnimationDefinition();
	}
}

// Finalizer
void wren_SpriteAnimationDefinitionFinalize(void* data)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)data;
	spriteAnimation->~WrenSpriteAnimationDefinition();
}

void wren_SpriteAnimationGetStartFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, spriteAnimation->spriteAnimation.startFrame);
}

void wren_SpriteAnimationSetStartFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.startFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
}

void wren_SpriteAnimationGetEndFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, spriteAnimation->spriteAnimation.endFrame);
}

void wren_SpriteAnimationSetEndFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.endFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
}

void wren_SpriteAnimationGetAnimationTime(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, spriteAnimation->spriteAnimation.animationTime);
}

void wren_SpriteAnimationSetAnimationTime(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.animationTime = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

void wren_SpriteAnimationGetLoop(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, spriteAnimation->spriteAnimation.loop);
}

void wren_SpriteAnimationSetLoop(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.loop = wrenGetSlotBool(vm, 1);
}

// Register Quat foreign class
WREN_FOREIGN_CLASS("animation", "SpriteAnimationDefinition", wren_SpriteAnimationDefinitionAllocate, wren_SpriteAnimationDefinitionFinalize, "SpriteAnimationDefinition class for defining the frames of a sprite sheet and speed of a sprite animation");

// Register constructors
WREN_CONSTRUCTOR_DOC("animation", "SpriteAnimationDefinition", wren_SpriteAnimationDefinitionAllocate, "Create empty SpriteAnimationDefinition", );
WREN_CONSTRUCTOR_DOC("animation", "SpriteAnimationDefinition", wren_SpriteAnimationDefinitionAllocate, "Create SpriteAnimationDefinition with startFrame, endFrame, animationTime, loop components", startFrame, endFrame, animationTime, loop);

// Register methods
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "startFrame", wren_SpriteAnimationGetStartFrame, "Get startFrame");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "startFrame=(_)", wren_SpriteAnimationSetStartFrame, "Set startFrame");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "endFrame", wren_SpriteAnimationGetEndFrame, "Get endFrame");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "endFrame=(_)", wren_SpriteAnimationSetEndFrame, "Set endFrame");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "animationTime", wren_SpriteAnimationGetAnimationTime, "Get animationTime");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "animationTime=(_)", wren_SpriteAnimationSetAnimationTime, "Set animationTime");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "loop", wren_SpriteAnimationGetLoop, "Get loop");
WREN_CLASS_METHOD("animation", "SpriteAnimationDefinition", "loop=(_)", wren_SpriteAnimationSetLoop, "Set loop");
