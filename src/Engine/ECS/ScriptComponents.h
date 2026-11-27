#pragma once

// The components scripts can reach (GameObject.hasComponent, <Component>.get, ...): each library's list, joined into
// COMPONENT_LIST. An entry is COMPONENT(component_name, component_type, component_name_string):
//   component_name: builds the generated Wren##component_name wrapper struct and binding names.
//   component_type: the Struktur::Component:: type behind it - usually the same as component_name, except
//     LocalTransform and WorldTransform, which share Component::Transform.
//   component_name_string: the name scripts use for the component.
// A game adds its own with a header that defines GAME_COMPONENT_LIST, named by STRUKTUR_GAME_COMPONENT_HEADER in its
// CMake.
#include "Engine/ECS/EcsComponentList.h"
#ifdef STRUKTUR_PHYSICS
	#include "Engine/Physics/PhysicsComponentList.h"
#else
	#define PHYSICS_COMPONENT_LIST
#endif
#ifdef STRUKTUR_GAME_COMPONENT_HEADER
	#include STRUKTUR_GAME_COMPONENT_HEADER
#else
	#define GAME_COMPONENT_LIST
#endif

#define COMPONENT_LIST ECS_COMPONENT_LIST PHYSICS_COMPONENT_LIST GAME_COMPONENT_LIST
