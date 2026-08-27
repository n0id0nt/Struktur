#pragma once

#include "ComponentListXMacro.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/ParticleEmitter.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/Component/WrenScript.h"

#define COMPONENT(component_name, component_type, component_name_string)                  \
	struct Wren##component_name                                                           \
	{                                                                                     \
		entt::entity entity                            = entt::null;                      \
		Struktur::Component::component_type* component = nullptr;                         \
		Wren##component_name()                                                            \
		    : component(nullptr),                                                         \
		      entity(entt::null)                                                          \
		{                                                                                 \
		}                                                                                 \
		Wren##component_name(entt::entity entity, Struktur::Component::component_type* v) \
		    : component(v),                                                               \
		      entity(entity)                                                              \
		{                                                                                 \
		}                                                                                 \
	};
COMPONENT_LIST
#undef COMPONENT
