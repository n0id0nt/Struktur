#pragma once

// component_name: used to build the generated Wren##component_name wrapper struct name.
// component_type: the actual Struktur::Component:: type backing it - usually equal to
//   component_name, except LocalTransform/WorldTransform which both share Component::Transform.
// component_name_string: the name scripts use to refer to the component (e.g. GameObject.hasComponent(...)).
#define COMPONENT_LIST                                             \
	COMPONENT(Camera, Camera, "Camera")                            \
	COMPONENT(Level, Level, "Level")                               \
	COMPONENT(World, World, "World")                               \
	COMPONENT(ParticleEmitter, ParticleEmitter, "ParticleEmitter") \
	COMPONENT(PhysicsBody, PhysicsBody, "PhysicsBody")             \
	COMPONENT(Shader, Shader, "Shader")                            \
	COMPONENT(Sprite, Sprite, "Sprite")                            \
	COMPONENT(SpriteAnimation, SpriteAnimation, "SpriteAnimation") \
	COMPONENT(TileMap, TileMap, "TileMap")                         \
	COMPONENT(LocalTransform, Transform, "LocalTransform")         \
	COMPONENT(WorldTransform, Transform, "WorldTransform")         \
	COMPONENT(WrenScript, WrenScript, "Script")                    \
	// COMPONENT_LIST
