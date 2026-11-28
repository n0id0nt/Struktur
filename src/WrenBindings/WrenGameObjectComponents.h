#pragma once

#include "Engine/ECS/ScriptComponents.h"
#include "wren.hpp"

#define COMPONENT(component_name, component_type, component_name_string)                                 \
	struct Wren##component_name                                                                          \
	{                                                                                                    \
		entt::entity entity                            = entt::null;                                     \
		Struktur::Component::component_type* component = nullptr;                                        \
		Wren##component_name()                                                                           \
		    : component(nullptr),                                                                        \
		      entity(entt::null)                                                                         \
		{                                                                                                \
		}                                                                                                \
		Wren##component_name(entt::entity entity, Struktur::Component::component_type* componentPointer) \
		    : component(componentPointer),                                                               \
		      entity(entity)                                                                             \
		{                                                                                                \
		}                                                                                                \
	};
COMPONENT_LIST
#undef COMPONENT

// Every script component's allocate, finalize and get(entity) bindings (defined in WrenGameObjectComponents.cpp), for
// the library that registers the component's class.
#define COMPONENT(component_name, component_type, component_name_string) \
	void wren_##component_name##Allocate(WrenVM* vm);                    \
	void wren_##component_name##Finalize(void* data);                    \
	void wren_##component_name##Get(WrenVM* vm);
COMPONENT_LIST
#undef COMPONENT
