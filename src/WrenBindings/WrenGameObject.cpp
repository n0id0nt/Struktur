#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

#include "ComponentListXMacro.h"

#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Identifier.h"

// ============================================================================
// GAME OBJECT MANAGER BINDINGS
// ============================================================================

// GameObject.create(name, parent) -> entityId
void wren_GameObjectCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameObjectManager = context->GetGameObjectManager();

	const char* name = wrenGetSlotString(vm, 1);
	double parentId = wrenGetSlotDouble(vm, 2);
	entt::entity parent = static_cast<entt::entity>(parentId);

	entt::entity entity = gameObjectManager.CreateGameObject(*context, name, parent);

	wrenSetSlotDouble(vm, 0, static_cast<double>(entity));
}

// GameObject.destroy(entity)
void wren_GameObjectDestroy(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameObjectManager = context->GetGameObjectManager();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	gameObjectManager.SafeDeleteGameObject(*context, entity);
}

// GameObject.isValid(entity) -> bool
void wren_GameObjectIsValid(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	bool valid = registry.valid(entity);
	wrenSetSlotBool(vm, 0, valid);
}

// GameObject.hasComponent(entity, componentName) ->bool
void wren_GameObjecthasComponent(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	const char* componentName = wrenGetSlotString(vm, 2);
	bool hasComponent = false;
#define COMPONENT(component_name, component_name_string) 										\
	if (strcmp(componentName, component_name_string) == 0) 						                \
	{																		                    \
        auto* componentValue = registry.try_get<Struktur::Component::component_name>(entity);   \
        hasComponent = componentValue != nullptr;										        \
    } else
	COMPONENT_LIST
#undef COMPONENT
    // need to handle last else statement
	{
        DEBUG_ERROR("%s is not a valid component type", componentName);
	}
	wrenSetSlotBool(vm, 1, hasComponent);
}

// GameObject.getAllWithComponent(componentName) -> List
void wren_GameObjectGetAllWithComponent(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	const char* componentName = wrenGetSlotString(vm, 1);
    
	wrenSetSlotNewList(vm, 0);
    
	int index = 0;
    
#define COMPONENT(component_name, component_name_string) 					\
	if (strcmp(componentName, #component_name_string) == 0) 				\
	{																		\
        auto view = registry.view<Struktur::Component::component_name>();	\
        for (auto entity : view)											\
		{																	\
            wrenSetSlotDouble(vm, 1, static_cast<double>(entity));			\
            wrenInsertInList(vm, 0, index, 1);								\
            index++;														\
        }																	\
    } else
	COMPONENT_LIST
#undef COMPONENT
    // need to handle last else statement
	{
        DEBUG_ERROR("%s is not a valid component type", componentName);
	}
}

// GameObject.getAllWithComponents(componentList) -> List
void wren_GameObjectGetAllWithComponents(WrenVM* vm)
    {
        Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
        auto& registry = context->GetRegistry();
        
        // Slot 1 contains a Wren list of component names
        if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
        {
            DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}

	int componentCount = wrenGetListCount(vm, 1);

	wrenEnsureSlots(vm, 3);
	// Get component names from list
	std::vector<std::string> components;
	for (int i = 0; i < componentCount; i++) {
		wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
		const char* componentName = wrenGetSlotString(vm, 2);
		components.emplace_back(componentName);
	}

	// Now filter entities based on components
	wrenSetSlotNewList(vm, 0);
	int resultIndex = 0;

	// Iterate all entities and check if they have ALL required components
	registry.each([&](auto entity)
		{
			bool hasAllComponents = true;

			for (const auto& compName : components)
			{
#define COMPONENT(component_name, component_name_string) 							\
            if (compName == component_name_string)                                  \
			{																		\
				if (!registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAllComponents = false;										\
                    break;															\
                }																	\
			} else
				COMPONENT_LIST
#undef COMPONENT
					// need to handle last else statement
				{
					DEBUG_ERROR("%s is not a valid component type", compName.c_str());
				}
			}

			if (hasAllComponents) {
				wrenSetSlotDouble(vm, 2, static_cast<double>(entity));
				wrenInsertInList(vm, 0, resultIndex, 2);
				resultIndex++;
			}
		});
}

// GameObject.getAnyWithComponents(componentList) -> List
void wren_GameObjectGetAnyWithComponents(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}

	int componentCount = wrenGetListCount(vm, 1);

	std::vector<std::string> components(componentCount);
	for (int i = 0; i < componentCount; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		components.emplace_back(wrenGetSlotString(vm, 2));
	}

	wrenSetSlotNewList(vm, 0);
	int resultIndex = 0;

	registry.each([&](auto entity) {
		bool hasAnyComponent = false;

		for (const auto& compName : components)
		{
#define COMPONENT(component_name, component_name_string) 							\
	        if (compName == component_name_string)                                  \
			{																		\
				if (!registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAnyComponent = false;										\
                    break;															\
                }																	\
			} else
			COMPONENT_LIST
#undef COMPONENT
				// need to handle last else statement
			{
				DEBUG_ERROR("%s is not a valid component type", compName.c_str());
			}
		}

		if (hasAnyComponent) {
			wrenSetSlotDouble(vm, 2, static_cast<double>(entity));
			wrenInsertInList(vm, 0, resultIndex, 2);
			resultIndex++;
		}
		});
}

// GameObject.getAll() -> List of all entities
void wren_GameObjectGetAll(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	wrenSetSlotNewList(vm, 0);

	int index = 0;

	// Iterate ALL entities
	registry.each([&](const auto& entity)
		{
			wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
			wrenInsertInList(vm, 0, index, 1);
			index++;
		});
}

// GameObject.getAllWithIdentifier(Identifier) -> List of entity IDs
void wren_GameObjectGetAllWithIdentifier(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	const char* tag = wrenGetSlotString(vm, 1);

	// Create a Wren list for results
	wrenSetSlotNewList(vm, 0);

	// Find all entities with matching tag/component
	auto view = registry.view<Struktur::Component::Identifier>();

	int index = 0;
	for (const auto& [entity, identifier] : view.each())
	{
		if (identifier.type == tag)
		{
			// Add entity ID to list
			wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
			wrenInsertInList(vm, 0, index, 1);
			index++;
		}
	}

	// Return list (already in slot 0)
}

// GameObject.forEach(callback) - calls callback for each entity
void wren_GameObjectForEach(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	// Callback is in slot 1
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 1);

	// Iterate all entities
	registry.each([vm, callbackHandle](auto entity) {
		// Call Wren callback with entity ID
		wrenEnsureSlots(vm, 2);
		wrenSetSlotHandle(vm, 0, callbackHandle);
		wrenSetSlotDouble(vm, 1, static_cast<double>(entity));

		wrenCall(vm, callbackHandle);
		});

	wrenReleaseHandle(vm, callbackHandle);
}

// GameObject.forEachWithComponent(componentName, callback)
void wren_GameObjectForEachWithComponent(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	const char* componentName = wrenGetSlotString(vm, 1);
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 2);

#define COMPONENT(component_name, component_name_string) 					\
	if (strcmp(componentName, component_name_string) == 0) 					\
	{																		\
        auto view = registry.view<Struktur::Component::component_name>();	\
        for (auto entity : view)											\
		{																	\
			wrenEnsureSlots(vm, 2);											\
            wrenSetSlotHandle(vm, 0, callbackHandle);						\
            wrenSetSlotDouble(vm, 1, static_cast<double>(entity));			\
            wrenCall(vm, callbackHandle);									\
        }																	\
    } else
	COMPONENT_LIST
#undef COMPONENT
		// need to handle last else statement
	{
		DEBUG_ERROR("%s is not a valid component type", componentName);
	}

	wrenReleaseHandle(vm, callbackHandle);
}

// GameObject.forEachWithComponents(componentName, callback)
void wren_GameObjectForEachWithComponents(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 2);

	int componentCount = wrenGetListCount(vm, 1);

	// Get component names from list
	std::vector<std::string> components(componentCount);
	for (int i = 0; i < componentCount; i++) {
		wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
		const char* componentName = wrenGetSlotString(vm, 2);
		components.emplace_back(componentName);
	}

	// Iterate all entities and check if they have ALL required components
	registry.each([&](auto entity)
		{
			bool hasAllComponents = true;

			for (const auto& compName : components)
			{
#define COMPONENT(component_name, component_name_string) 							\
			if (compName == component_name_string) 									\
			{																		\
				if (!registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAllComponents = false;										\
                    break;															\
                }																	\
			} else
				COMPONENT_LIST
#undef COMPONENT
					// need to handle last else statement
				{
					DEBUG_ERROR("%s is not a valid component type", compName.c_str());
				}
			}

			if (hasAllComponents) {
				wrenEnsureSlots(vm, 2);
				wrenSetSlotHandle(vm, 0, callbackHandle);
				wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
				wrenCall(vm, callbackHandle);
			}
		});

	wrenReleaseHandle(vm, callbackHandle);
}

// GameObject.forEachWithAnyComponents(componentName, callback)
void wren_GameObjectForEachWithAnyComponents(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 2);

	int componentCount = wrenGetListCount(vm, 1);

	// Get component names from list
	std::vector<std::string> components(componentCount);
	for (int i = 0; i < componentCount; i++) {
		wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
		const char* componentName = wrenGetSlotString(vm, 2);
		components.emplace_back(componentName);
	}

	// Iterate all entities and check if they have ANY required components
	registry.each([&](auto entity)
		{
			bool hasAnyComponents = false;

			for (const auto& compName : components)
			{
#define COMPONENT(component_name, component_name_string) 							\
			if (compName == component_name_string) 									\
			{																		\
				if (registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAnyComponents = true;										\
                    break;															\
                }																	\
			} else
				COMPONENT_LIST
#undef COMPONENT
					// need to handle last else statement
				{
					DEBUG_ERROR("%s is not a valid component type", compName.c_str());
				}
			}

			if (hasAnyComponents) {
				wrenEnsureSlots(vm, 2);
				wrenSetSlotHandle(vm, 0, callbackHandle);
				wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
				wrenCall(vm, callbackHandle);
			}
		});

	wrenReleaseHandle(vm, callbackHandle);
}

WREN_CLASS_STATIC("gameObject", "GameObject", "destroy(_)", wren_GameObjectDestroy, "Destroy an Game Object and all its children.");
WREN_CLASS_STATIC("gameObject", "GameObject", "create(_,_)", wren_GameObjectCreate, "Create a new Game Object with the given name and parent. Returns entity ID.");
WREN_CLASS_STATIC("gameObject", "GameObject", "isValid(_)", wren_GameObjectIsValid, "Check if an entity ID is valid.");
WREN_CLASS_STATIC("gameObject", "GameObject", "hasComponent(_)", wren_GameObjecthasComponent, "Checks if entity has a specific component.");
WREN_CLASS_STATIC("gameObject", "GameObject", "getAllWithComponent(_)", wren_GameObjectGetAllWithComponent, "Get all entities with a specific component. Returns list of entity IDs.");
WREN_CLASS_STATIC("gameObject", "GameObject", "getAllWithComponents(_)", wren_GameObjectGetAllWithComponents, "Get all entities with ALL specified components. Pass list of component names.");
WREN_CLASS_STATIC("gameObject", "GameObject", "getAllWithAnyComponents(_)", wren_GameObjectGetAnyWithComponents, "Get entities with ANY of the specified components.");
WREN_CLASS_STATIC("gameObject", "GameObject", "getAll()", wren_GameObjectGetAll, "Get all entities in the registry. Returns list of entity IDs.");
WREN_CLASS_STATIC("gameObject", "GameObject", "getAllWithIdentifier(_)", wren_GameObjectGetAllWithIdentifier, "Get all entities with a specific identifier. Returns list of entity IDs.");
WREN_CLASS_STATIC("gameObject", "GameObject", "forEach(_)", wren_GameObjectForEach, "Iterate all entities, calling callback for each.");
WREN_CLASS_STATIC("gameObject", "GameObject", "forEachWithComponent(_,_)",wren_GameObjectForEachWithComponent, "Iterate entities with component, calling callback for each.");
WREN_CLASS_STATIC("gameObject", "GameObject", "forEachWithComponents(_,_)", wren_GameObjectForEachWithComponents, "Iterate entities with component, calling callback for each.");
WREN_CLASS_STATIC("gameObject", "GameObject", "forEachWithAnyComponents(_,_)", wren_GameObjectForEachWithAnyComponents, "Iterate entities with component, calling callback for each.");
