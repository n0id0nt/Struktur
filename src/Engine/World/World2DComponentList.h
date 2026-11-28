#pragma once

// The 2D world library's script components (see Engine/ECS/ScriptComponents.h).
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/ParticleEmitter.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/World.h"

#define WORLD2D_COMPONENT_LIST                                     \
	COMPONENT(Camera, Camera, "Camera")                            \
	COMPONENT(Level, Level, "Level")                               \
	COMPONENT(World, World, "World")                               \
	COMPONENT(ParticleEmitter, ParticleEmitter, "ParticleEmitter") \
	COMPONENT(Shader, Shader, "Shader")                            \
	COMPONENT(Sprite, Sprite, "Sprite")                            \
	COMPONENT(SpriteAnimation, SpriteAnimation, "SpriteAnimation") \
	COMPONENT(TileMap, TileMap, "TileMap")
