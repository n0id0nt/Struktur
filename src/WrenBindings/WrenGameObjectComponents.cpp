#include "wrenGameObjectComponents.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"

#include "Engine/Game/Level.h"

#include "WrenMath.h"
#include "WrenResourceHandles.h"
#include "WrenPhysics.h"
#include "WrenAnimation.h"
#include "ComponentListXMacro.h"

// ============================================================================
// COMPONENT ALLOCATOR BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_name_string) 					                                                \
    void wren_##component_name##Allocate(WrenVM* vm)                                                                        \
    {                                                                                                                       \
        wrenGetVariable(vm, "gameObjectComponents", component_name_string, 0);                                              \
        Wren##component_name* value = (Wren##component_name*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(Wren##component_name)); \
        new (value) Wren##component_name();                                                                                 \
    }
    COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// COMPONENT FINALIZER BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_name_string) 		   \
    void wren_##component_name##Finalize(void* data)               \
    {                                                              \
        Wren##component_name* value = (Wren##component_name*)data; \
        value->~Wren##component_name();                            \
    }
    COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// COMPONENT GET BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_name_string) 		                                                                        \
    void wren_##component_name##Get(WrenVM* vm)                                                                                         \
    {                                                                                                                                   \
        Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));                                      \
        entt::registry& registry = context->GetRegistry();                                                                              \
        double entityId = wrenGetSlotDouble(vm, 1);                                                                                     \
        entt::entity entity = static_cast<entt::entity>(entityId);                                                                      \
        Struktur::Component::component_name* component = registry.try_get<Struktur::Component::component_name>(entity);                 \
        if (component)                                                                                                                      \
        {                                                                                                                               \
            wrenGetVariable(vm, "gameObjectComponents", component_name_string, 1);                                                      \
            Wren##component_name* wrenComponent = (Wren##component_name*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(Wren##component_name)); \
            new (wrenComponent) Wren##component_name(entity, component);                                                                \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            wrenSetSlotNull(vm, 0);                                                                                                     \
        }                                                                                                                               \
    }
    COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// CAMERA BINDINGS
// ============================================================================

// Camera.create(entity) -> Camera
void wren_CameraCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto& cameraComponent = registry.emplace<Struktur::Component::Camera>(entity);

	wrenGetVariable(vm, "gameObjectComponents", "Camera", 1);  // Get class into slot 1
	WrenCamera* camera = (WrenCamera*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenCamera));

	// Camera.new() - identity
	new (camera) WrenCamera(entity, &cameraComponent);
}

void wren_CameraGetZoom(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, camera->component->zoom);
}

void wren_CameraSetZoom(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->component->zoom = (float)wrenGetSlotDouble(vm, 1);
}

void wren_CameraGetForcePosition(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, camera->component->forcePosition);
}

void wren_CameraSetForcePosition(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->component->forcePosition = wrenGetSlotBool(vm, 1);
}

void wren_CameraGetDamping(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "gameObjectComponents", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(camera->component->damping);
}

void wren_CameraSetDamping(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	WrenVec2* damping = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	camera->component->damping = damping->value;
}

// Camera.worldPosToScreenPos(worldPos) -> Vec2
void wren_CameraWorldPosToScreenPos(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::GameResource::Camera& camera = context->GetCamera();

	WrenVec2* worldPos = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	glm::vec2 screenPos = camera.WorldPosToScreenPos(worldPos->value);

	wrenGetVariable(vm, "gameObjectComponents", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(screenPos);
}

// Camera.screenPosToWorldPos(worldPos) -> Vec2
void wren_CameraScreenPosToWorldPos(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::GameResource::Camera& camera = context->GetCamera();

	WrenVec2* screenPos = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	glm::vec2 worldPos = camera.ScreenPosToWorldPos(screenPos->value);

	wrenGetVariable(vm, "gameObjectComponents", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(worldPos);
}

// Register Camera Component foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "Camera", wren_CameraAllocate, wren_CameraFinalize, "Camera component class");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "Camera", "zoom", wren_CameraGetZoom, "Get the zoom");
WREN_CLASS_METHOD("gameObjectComponents", "Camera", "zoom=(_)", wren_CameraSetZoom, "Set the zoom");
WREN_CLASS_METHOD("gameObjectComponents", "Camera", "forcePosition", wren_CameraGetForcePosition, "Get the forcePosition, will directly set the position of the next frame");
WREN_CLASS_METHOD("gameObjectComponents", "Camera", "forcePosition=(_)", wren_CameraSetForcePosition, "Set the forcePosition, will directly set the position of the next frame");
WREN_CLASS_METHOD("gameObjectComponents", "Camera", "damping", wren_CameraGetDamping, "Get the damping");
WREN_CLASS_METHOD("gameObjectComponents", "Camera", "damping=(_)", wren_CameraSetDamping, "Set the damping");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "Camera", "create(_)", wren_CameraCreate, "Creates a camera component.");
WREN_CLASS_STATIC("gameObjectComponents", "Camera", "get(_)", wren_CameraGet, "Gets a camera component.");
WREN_CLASS_STATIC("gameObjectComponents", "Camera", "worldPosToScreenPos(_)", wren_CameraWorldPosToScreenPos, "Converts a world position to the screen position from the currently active camera.");
WREN_CLASS_STATIC("gameObjectComponents", "Camera", "screenPosToWorldPos(_)", wren_CameraScreenPosToWorldPos, "Converts a screen position to the world position from the currently active camera.");

// ============================================================================
// LEVEL BINDINGS
// ============================================================================

void wren_LevelGetIndex(WrenVM* vm)
{
	WrenLevel* level = (WrenLevel*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, level->component->index);
}

void wren_LevelGetWidth(WrenVM* vm)
{
	WrenLevel* level = (WrenLevel*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, level->component->width);
}

void wren_LevelGetHeight(WrenVM* vm)
{
	WrenLevel* level = (WrenLevel*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, level->component->height);
}

// Register Level Component foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "Level", wren_LevelAllocate, wren_LevelFinalize, "Level component class");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "Level", "index", wren_LevelGetIndex, "Get the level index");
WREN_CLASS_METHOD("gameObjectComponents", "Level", "width", wren_LevelGetWidth, "Get the level width");
WREN_CLASS_METHOD("gameObjectComponents", "Level", "height", wren_LevelGetHeight, "Get the level height");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "Level", "get(_)", wren_LevelGet, "Gets a level component.");

// ============================================================================
// WORLD BINDINGS
// ============================================================================

// World.loadLevelEntities(levelIndex) -> number
void wren_WorldLoadLevelEntities(WrenVM* vm)
{
    WrenWorld* world = (WrenWorld*)wrenGetSlotForeign(vm, 0);

    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    
	double levelDouble = wrenGetSlotDouble(vm, 1);
	int levelIndex = static_cast<int>(levelDouble);
    
	entt::entity levelEntity = Struktur::GameResource::Level::LoadLevelEntities(*context, world->entity, levelIndex);

    if (levelEntity == entt::null)
    {
        wrenSetSlotNull(vm, 0);
    }
    else
    {
        double entityId = static_cast<double>(levelEntity);
        wrenSetSlotDouble(vm, 0, entityId);
    }
}

// World.loadLevelEntities(worldEntity, levelIndex) -> number
void wren_WorldStaticLoadLevelEntities(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity worldEntity = static_cast<entt::entity>(entityId);
    
	double levelDouble = wrenGetSlotDouble(vm, 2);
	int levelIndex = static_cast<int>(levelDouble);
    
	entt::entity levelEntity = Struktur::GameResource::Level::LoadLevelEntities(*context, worldEntity, levelIndex);
    
	if (levelEntity == entt::null)
    {
        wrenSetSlotNull(vm, 0);
    }
    else
    {
        double entityId = static_cast<double>(levelEntity);
        wrenSetSlotDouble(vm, 0, entityId);
    }
}

// World.createWorldEntity(worldFilePath) -> number
void wren_WorldCreateWorldEntity(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    const char* worldFilePath = wrenGetSlotString(vm, 1);
    entt::entity worldEntity = Struktur::GameResource::Level::CreateWorldEntity(*context, worldFilePath);
    double entityId = static_cast<double>(worldEntity);
    wrenSetSlotDouble(vm, 0, entityId);
}

// World.getLevelIndex(levelName) -> number
void wren_WorldGetLevelIndex(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    
    WrenWorld* world = (WrenWorld*)wrenGetSlotForeign(vm, 0);
	const char* levelName = wrenGetSlotString(vm, 1);

	Struktur::FileLoading::LevelParser::World& worldMap = world->component->worldMap;

	int index = -1;
	for (int i = 0; i < worldMap.levels.size(); i++)
	{
		if (worldMap.levels[i].identifier == levelName)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	double doubleNumber = static_cast<double>(index);

	wrenSetSlotDouble(vm, 0, doubleNumber);
}

// World.getLevelIndex(entity, levelName) -> number
void wren_WorldStaticGetLevelIndex(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* levelName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);

	if (!worldComponent)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	Struktur::FileLoading::LevelParser::World& worldMap = worldComponent->worldMap;

	int index = -1;
	for (int i = 0; i < worldMap.levels.size(); i++)
	{
		if (worldMap.levels[i].identifier == levelName)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	double doubleNumber = static_cast<double>(index);

	wrenSetSlotDouble(vm, 0, doubleNumber);
}


// Register Level Component foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "World", wren_WorldAllocate, wren_WorldFinalize, "World component class");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "World", "loadLevelEntities(_)", wren_WorldLoadLevelEntities, "Creates a level in the game and all its corresponding objects and entities.");
WREN_CLASS_STATIC("gameObjectComponents", "World", "getLevelIndex(_)", wren_WorldGetLevelIndex, "Get the index of an Level in the world.");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "World", "get(_)", wren_WorldGet, "Gets a world component.");
WREN_CLASS_STATIC("gameObjectComponents", "World", "loadLevelEntities(_,_)", wren_WorldStaticLoadLevelEntities, "Creates a level in the game and all its corresponding objects and entities.");
WREN_CLASS_STATIC("gameObjectComponents", "World", "createWorldEntity(_)", wren_WorldCreateWorldEntity, "Loads in a LDTK world file and creates the world game object and corresponding components.");
WREN_CLASS_STATIC("gameObjectComponents", "World", "getLevelIndex(_,_)", wren_WorldStaticGetLevelIndex, "Get the index of an Level in the world.");

// ============================================================================
// PHYSICS BODY BINDINGS
// ============================================================================

// PhysicsBody.create(entity, bodyDef, shape) -> PhysicsBody
void wren_PhysicsBodyCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 2));
	WrenPhysicsCircleShape* shape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 3));

	Struktur::Component::PhysicsBody& physicsBodyComponent = physicsSystem.CreatePhysicsBody(*context, entity, bodyDef->bodyDef, shape->physicsShape);

	wrenGetVariable(vm, "game", "PhysicsBody", 1);  // Get class into slot 1
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenPhysicsBody));

	// PhysicsBody.new() - identity
	new (physicsBody) WrenPhysicsBody(entity, &physicsBodyComponent);
}

void wren_PhysicsBodySetFixedRotation(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	bool fixedRotation = (float)wrenGetSlotBool(vm, 1);
	physicsBody->component->body->SetFixedRotation(fixedRotation);
}

void wren_PhysicsBodyGetSyncFromPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, physicsBody->component->syncFromPhysics);
}

void wren_PhysicsBodySetSyncFromPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	physicsBody->component->syncFromPhysics = (float)wrenGetSlotBool(vm, 1);
}

void wren_PhysicsBodyGetSyncToPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, physicsBody->component->syncToPhysics);
}

void wren_PhysicsBodySetSyncToPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	physicsBody->component->syncToPhysics = (float)wrenGetSlotBool(vm, 1);
}

// PhysicsBody.setLinearVelocity(entity, velocity)
void wren_PhysicsBodyStaticSetLinearVelocity(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenVec2* velocity = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

	auto* physicsBodyComponent = registry.try_get<Struktur::Component::PhysicsBody>(entity);

	if (!physicsBodyComponent)
	{
		return;
	}

	b2Vec2 b2Velecity = b2Vec2(velocity->value.x, velocity->value.y);
	physicsBodyComponent->body->SetLinearVelocity(b2Velecity);
}

// PhysicsBody.linearVelocity = velocity
void wren_PhysicsBodySetLinearVelocity(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);

	WrenVec2* velocity = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));

	b2Vec2 b2Velecity = b2Vec2(velocity->value.x, velocity->value.y);
	physicsBody->component->body->SetLinearVelocity(b2Velecity);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "PhysicsBody", wren_PhysicsBodyAllocate, wren_PhysicsBodyFinalize, "PhysicsBody class wraps PhysicsBody component");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "PhysicsBody", "fixedRotation=(_)", wren_PhysicsBodySetFixedRotation, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("gameObjectComponents", "PhysicsBody", "syncFromPhysics", wren_PhysicsBodyGetSyncFromPhysics, "Get if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("gameObjectComponents", "PhysicsBody", "syncFromPhysics=(_)", wren_PhysicsBodySetSyncFromPhysics, "Set if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("gameObjectComponents", "PhysicsBody", "syncToPhysics", wren_PhysicsBodyGetSyncToPhysics, "Get if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("gameObjectComponents", "PhysicsBody", "syncToPhysics=(_)", wren_PhysicsBodySetSyncToPhysics, "Set if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("gameObjectComponents", "PhysicsBody", "linearVelocity", wren_PhysicsBodySetLinearVelocity, "Sets the linear velocity of a physics body.");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "PhysicsBody", "create(_,_,_)", wren_PhysicsBodyCreate, "Create a physics body");
WREN_CLASS_STATIC("gameObjectComponents", "PhysicsBody", "get(_)", wren_PhysicsBodyGet, "Gets a physics body");
WREN_CLASS_STATIC("gameObjectComponents", "PhysicsBody", "setLinearVelocity(_,_)", wren_PhysicsBodyStaticSetLinearVelocity, "Sets the linear velocity of a physics body.");

// ============================================================================
// SHADER BINDINGS
// ============================================================================

// Shader.create(entity) -> Shader
void wren_ShaderCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto& shaderComponent = registry.emplace<Struktur::Component::Shader>(entity);

	wrenGetVariable(vm, "gameObjectComponents", "Shader", 1);  // Get class into slot 1
	WrenShader* shader = (WrenShader*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenShader));

	// Shader.new() - identity
	new (shader) WrenShader(entity, &shaderComponent);
}

void wren_ShaderSetFloatUniform(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::System::SystemManager& systemManager = context->GetSystemManager();
    auto& shaderSystem = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = (WrenShader*)wrenGetSlotForeign(vm, 0);
    const char* name = wrenGetSlotString(vm, 1);
    float value = static_cast<float>(wrenGetSlotDouble(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value);
}

void wren_ShaderSetIntUniform(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::System::SystemManager& systemManager = context->GetSystemManager();
    auto& shaderSystem = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = (WrenShader*)wrenGetSlotForeign(vm, 0);
    const char* name = wrenGetSlotString(vm, 1);
    int value = static_cast<int>(wrenGetSlotDouble(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value);
}

void wren_ShaderSetVec2Uniform(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::System::SystemManager& systemManager = context->GetSystemManager();
    auto& shaderSystem = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = (WrenShader*)wrenGetSlotForeign(vm, 0);
    const char* name = wrenGetSlotString(vm, 1);
    WrenVec2* value = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

void wren_ShaderSetVec3Uniform(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::System::SystemManager& systemManager = context->GetSystemManager();
    auto& shaderSystem = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = (WrenShader*)wrenGetSlotForeign(vm, 0);
    const char* name = wrenGetSlotString(vm, 1);
    WrenVec3* value = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

void wren_ShaderSetVec4Uniform(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::System::SystemManager& systemManager = context->GetSystemManager();
    auto& shaderSystem = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = (WrenShader*)wrenGetSlotForeign(vm, 0);
    const char* name = wrenGetSlotString(vm, 1);
    WrenVec4* value = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

void wren_ShaderSetMat4Uniform(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::System::SystemManager& systemManager = context->GetSystemManager();
    auto& shaderSystem = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = (WrenShader*)wrenGetSlotForeign(vm, 0);
    const char* name = wrenGetSlotString(vm, 1);
    WrenMat4* value = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "Shader", wren_ShaderAllocate, wren_ShaderFinalize, "Shader class wraps Shader component");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "Shader", "SetFloatUniform(_)", wren_ShaderSetFloatUniform, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("gameObjectComponents", "Shader", "SetIntUniform(_)", wren_ShaderSetIntUniform, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("gameObjectComponents", "Shader", "SetVec2Uniform(_)", wren_ShaderSetVec2Uniform, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("gameObjectComponents", "Shader", "SetVec3Uniform(_)", wren_ShaderSetVec3Uniform, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("gameObjectComponents", "Shader", "SetVec4Uniform(_)", wren_ShaderSetVec4Uniform, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("gameObjectComponents", "Shader", "SetMat4Uniform(_)", wren_ShaderSetMat4Uniform, "Sets the physics body fixed rotation");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "Shader", "create(_)", wren_ShaderCreate, "Create a Shader body");
WREN_CLASS_STATIC("gameObjectComponents", "Shader", "get(_)", wren_ShaderGet, "Gets a Shader body");

// ============================================================================
// SPRITE ANIMATION BINDINGS
// ============================================================================

// SpriteAnimation.create(entity) -> SpriteAnimation
void wren_SpriteAnimationCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	Struktur::Component::SpriteAnimation& spriteAnimationComponent = registry.emplace<Struktur::Component::SpriteAnimation>(entity);

	wrenGetVariable(vm, "gameObjectComponents", "SpriteAnimation", 1);  // Get class into slot 1
	WrenSpriteAnimation* spriteAnimation = (WrenSpriteAnimation*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenSpriteAnimation));

	// SpriteAnimation.new() - identity
	new (spriteAnimation) WrenSpriteAnimation(entity, &spriteAnimationComponent);
}

// SpriteAnimation.addAnimation(animationKey, animationDefinition)
void wren_SpriteAnimationAddAnimation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	Struktur::System::AnimationSystem& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();

	auto* spriteAnimation = static_cast<WrenSpriteAnimation*>(wrenGetSlotForeign(vm, 0));
	const char* animationKey = wrenGetSlotString(vm, 1);
	auto* animationDefinition = static_cast<WrenSpriteAnimationDefinition*>(wrenGetSlotForeign(vm, 2));

	animationSystem.AddAnimation(*context, spriteAnimation->entity, animationKey, animationDefinition->spriteAnimation);
}

// SpriteAnimation.setCurrentAnimation(entity, animationName)
void wren_SpriteAnimationStaticSetCurrentAnimation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!animationSystem.IsAnimationPlaying(*context, entity, animationName))
	{
		animationSystem.PlayAnimation(*context, entity, animationName);
	}
}

// SpriteAnimation.playAnimation(entity, animationName)
void wren_SpriteAnimationPlayAnimation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	animationSystem.PlayAnimation(*context, entity, animationName);
}

// SpriteAnimation.isAnimationPlaying(entity, animationName) -> bool
void wren_SpriteAnimationIsAnimationPlaying(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	bool isAnimationPlaying = animationSystem.IsAnimationPlaying(*context, entity, animationName);
	wrenSetSlotBool(vm, 0, isAnimationPlaying);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "SpriteAnimation", wren_SpriteAnimationAllocate, wren_SpriteAnimationFinalize, "Sprite animation class wraps SpriteAnimation component");

// Register methods
WREN_CLASS_METHOD("game", "SpriteAnimation", "addAnimation(_,_)", wren_SpriteAnimationAddAnimation, "Adds an animation to the sprite animation component");

// Register static methods
WREN_CLASS_STATIC("game", "SpriteAnimation", "create(_)", wren_SpriteAnimationCreate, "Create a Sprite animation");
WREN_CLASS_STATIC("game", "SpriteAnimation", "get(_)", wren_SpriteAnimationGet, "Gets a Sprite animation");
WREN_CLASS_STATIC("game", "SpriteAnimation", "setCurrentAnimation(_,_)", wren_SpriteAnimationStaticSetCurrentAnimation, "Will set and play a current sprite animation, is already playing the animation continue it.");
WREN_CLASS_STATIC("game", "SpriteAnimation", "forcePlayAnimation(_,_)", wren_SpriteAnimationPlayAnimation, "Will play a sprite animation, and if playering animation will forcibly restart it.");
WREN_CLASS_STATIC("game", "SpriteAnimation", "isAnimationPlaying(_,_)", wren_SpriteAnimationIsAnimationPlaying, "Checks if a cirtain animation is playing.");


// ============================================================================
// SPRITE BINDINGS
// ============================================================================

// Sprite.create(spriteEntity, texture, color, offset, columns, rows, flipped, index, renderPriority) -> number
void wren_SpriteCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	WrenTextureHandle* texture = static_cast<WrenTextureHandle*>(wrenGetSlotForeign(vm, 2));
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* offset = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	int columns = static_cast<int>(wrenGetSlotDouble(vm, 5));
	int rows = static_cast<int>(wrenGetSlotDouble(vm, 6));
	bool flipped = wrenGetSlotBool(vm, 7);
	int index = static_cast<int>(wrenGetSlotDouble(vm, 8));
	int renderPriority = static_cast<int>(wrenGetSlotDouble(vm, 9));

	::Color rayColor{ (unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b, (unsigned char)color->value.a };

	registry.emplace<Struktur::Component::Sprite>(levelEntity, texture->resource, rayColor, offset->value, columns, rows, flipped, index, renderPriority);
}

void wren_SpriteGetTexture(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
	WrenTextureHandle* texture = (WrenTextureHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenTextureHandle));
    new (texture) WrenTextureHandle(sprite->component->texture);
}

void wren_SpriteSetTexture(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	WrenTextureHandle* texture = (WrenTextureHandle*)wrenGetSlotForeign(vm, 1);
    sprite->component->texture = texture->resource;
}

void wren_SpriteGetColor(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
	WrenVec4* color = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
    glm::vec4 glmColor((float)sprite->component->color.r, (float)sprite->component->color.g, (float)sprite->component->color.b, (float)sprite->component->color.a);
    new (color) WrenVec4(glmColor);
}

void wren_SpriteSetColor(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	WrenVec4* color = (WrenVec4*)wrenGetSlotForeign(vm, 1);
    ::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b, (unsigned char)color->value.a};
    sprite->component->color = rayColor;
}

void wren_SpriteGetOffset(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
	WrenVec2* offset = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
    new (offset) WrenVec2(sprite->component->offset);
}

void wren_SpriteSetOffset(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	WrenVec2* offset = (WrenVec2*)wrenGetSlotForeign(vm, 1);
    sprite->component->offset = offset->value;
}

void wren_SpriteGetColumns(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
    double columns = static_cast<double>(sprite->component->columns);
	wrenSetSlotDouble(vm, 0, columns);
}

void wren_SpriteSetColumns(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int columns = static_cast<int>(wrenGetSlotDouble(vm, 1));
    sprite->component->columns = columns;
}

void wren_SpriteGetRows(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
    double rows = static_cast<double>(sprite->component->rows);
	wrenSetSlotDouble(vm, 0, rows);
}

void wren_SpriteSetRows(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int rows = static_cast<int>(wrenGetSlotDouble(vm, 1));
    sprite->component->rows = rows;
}

void wren_SpriteGetIndex(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
    double index = static_cast<double>(sprite->component->index);
	wrenSetSlotDouble(vm, 0, index);
}

void wren_SpriteSetIndex(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int index = static_cast<int>(wrenGetSlotDouble(vm, 1));
    sprite->component->index = index;
}

void wren_SpriteGetRenderPriority(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
    double renderPriority = static_cast<double>(sprite->component->renderPriority);
	wrenSetSlotDouble(vm, 0, renderPriority);
}

void wren_SpriteSetRenderPriority(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int renderPriority = static_cast<int>(wrenGetSlotDouble(vm, 1));
    sprite->component->renderPriority = renderPriority;
}

void wren_SpriteGetFlipped(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
    wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
    bool flipped = sprite->component->flipped;
	wrenSetSlotBool(vm, 0, flipped);
}

void wren_SpriteSetFlipped(WrenVM* vm)
{
    WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	bool flipped = wrenGetSlotBool(vm, 1);
    sprite->component->flipped = flipped;
}

// Sprite.setRenderPriority(entity, renderPriority)
void wren_SpriteStaticSetRenderPriority(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	double renderPriority = wrenGetSlotDouble(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);
    
	if (!sprite)
	{
        return;
	}
    
	sprite->renderPriority = static_cast<int>(renderPriority);
}

// Sprite.setFlipped(entity, flipped)
void wren_SpriteStaticSetFlipped(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	bool flipped = wrenGetSlotBool(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->flipped = flipped;
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "Sprite", wren_SpriteAllocate, wren_SpriteFinalize, "Sprite animation class wraps SpriteAnimation component");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "texture", wren_SpriteGetTexture, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "texture=(_)", wren_SpriteSetTexture, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "color", wren_SpriteGetColor, "Sets the sprites color");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "color=(_)", wren_SpriteSetColor, "Sets the sprites color");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "offset", wren_SpriteGetOffset, "Sets the sprites offset");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "offset=(_)", wren_SpriteSetOffset, "Sets the sprites offset");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "columns", wren_SpriteGetColumns, "Sets the sprites columns");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "columns=(_)", wren_SpriteSetColumns, "Sets the sprites columns");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "rows", wren_SpriteGetRows, "Sets the sprites rows");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "rows=(_)", wren_SpriteSetRows, "Sets the sprites rows");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "flipped", wren_SpriteGetFlipped, "Sets the sprites flipped");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "flipped=(_)", wren_SpriteSetFlipped, "Sets the sprites flipped");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "index", wren_SpriteGetIndex, "Sets the sprites index");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "index=(_)", wren_SpriteSetIndex, "Sets the sprites index");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "renderPriority", wren_SpriteGetRenderPriority, "Sets the sprites render");
WREN_CLASS_METHOD("gameObjectComponents", "Sprite", "renderPriority=(_)", wren_SpriteSetRenderPriority, "Sets the sprites render");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "Sprite", "create(_,_,_,_,_,_,_,_,_)", wren_SpriteCreate, "Creates the sprite Component.");
WREN_CLASS_STATIC("gameObjectComponents", "Sprite", "get(_)", wren_SpriteGet, "Gets a Sprite animation");
WREN_CLASS_STATIC("gameObjectComponents", "Sprite", "setRenderPriority(_,_)", wren_SpriteStaticSetRenderPriority, "Sets the render priority of a sprite component");
WREN_CLASS_STATIC("gameObjectComponents", "Sprite", "setFlipped(_,_)", wren_SpriteStaticSetFlipped, "Flips a sprite in a horizontal direction");

// ============================================================================
// LOCAL TRANSFORM BINDINGS
// ============================================================================

// LocalTransform.getPosition() -> vec3 or null
void wren_LocalTransformGetPosition(WrenVM* vm)
{
    WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->position);
}

// LocalTransform.getPosition(entity) -> vec3 or null
void wren_LocalTransformStaticGetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->position);
}

// LocalTransform.setPosition(vec3)
void wren_LocalTransformSetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}
    
	transformSystem.SetLocalTransform(*context, transform->entity, vec->value, transform->component->scale, transform->component->rotation);
}

// LocalTransform.setPosition(entity, vec3)
void wren_LocalTransformStaticSetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}
    
    auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        DEBUG_ERROR("No Transform");
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, vec->value, transform->scale, transform->rotation);
}

// LocalTransform.getScale() -> vec3 or null
void wren_LocalTransformGetScale(WrenVM* vm)
{
    WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->scale);
}

// LocalTransform.getScale(entity) -> vec3 or null
void wren_LocalTransformStaticGetScale(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->scale);
}

// LocalTransform.setScale(vec3)
void wren_LocalTransformSetScale(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}
    
	transformSystem.SetLocalTransform(*context, transform->entity, transform->component->position, vec->value, transform->component->rotation);
}

// LocalTransform.setScale(entity, vec3)
void wren_LocalTransformStaticSetScale(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}
    
    auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        DEBUG_ERROR("No Transform");
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, transform->position, vec->value, transform->rotation);
}

// LocalTransform.getRotation() -> Quat or null
void wren_LocalTransformGetRotation(WrenVM* vm)
{
    WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "gameObjectComponents", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->component->rotation);
}

// LocalTransform.getRotation(entity) -> Quat or null
void wren_LocalTransformStaticGetRotation(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	wrenGetVariable(vm, "gameObjectComponents", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->rotation);
}

// LocalTransform.setRotation(Quat)
void wren_LocalTransformSetRotation(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));
    
	if (!quat)
	{
        DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}
    
	transformSystem.SetLocalTransform(*context, transform->entity, transform->component->position, transform->component->scale, quat->value);
}

// LocalTransform.setPosition(entity, Quat)
void wren_LocalTransformStaticSetRotation(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));
    
	if (!quat)
	{
        DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}
    
    auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        DEBUG_ERROR("Transform.setRotation: No Transform");
		return;
	}
    
	transformSystem.SetLocalTransform(*context, entity, transform->position, transform->scale, quat->value);
}

// LocalTransform.getMatrix() -> Mat4 or null
void wren_LocalTransformGetMatrix(WrenVM* vm)
{
    WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->component->matrix);
}

// LocalTransform.getMatrix(entity) -> Mat4 or null
void wren_LocalTransformStaticGetMatrix(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->matrix);
}

// LocalTransform.setMatrix(mat4)
void wren_LocalTransformSetMatrix(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));
    
	if (!mat)
	{
        DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}
    
	transformSystem.SetLocalTransform(*context, transform->entity, mat->value);
}

// LocalTransform.setMatrix(entity, mat4)
void wren_LocalTransformStaticSetMatrix(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));
    
	if (!mat)
	{
        DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}
    
	transformSystem.SetLocalTransform(*context, entity, mat->value);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "LocalTransform", wren_LocalTransformAllocate, wren_LocalTransformFinalize, "Sprite animation class wraps SpriteAnimation component");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "position", wren_LocalTransformGetPosition, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "position=(_)", wren_LocalTransformSetPosition, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "rotation", wren_LocalTransformGetRotation, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "rotation=(_)", wren_LocalTransformSetRotation, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "scale", wren_LocalTransformGetScale, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "scale=(_)", wren_LocalTransformSetScale, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "matrix", wren_LocalTransformGetMatrix, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "LocalTransform", "matrix=(_)", wren_LocalTransformSetMatrix, "Sets the sprites texture");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "get(_)", wren_LocalTransformGet, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "getPosition(_)", wren_LocalTransformStaticGetPosition, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "setPosition(_,_)", wren_LocalTransformStaticSetPosition, "Set the position of an entity.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "getRotation(_)", wren_LocalTransformStaticGetRotation, "Get rotation of entity as Quat.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "setRotation(_,_)", wren_LocalTransformStaticSetRotation, "Set rotation of entity from Quat.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "getScale(_)", wren_LocalTransformStaticGetScale, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "setScale(_,_)", wren_LocalTransformStaticSetScale, "Set the position of an entity.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "getMatrix(_)", wren_LocalTransformStaticGetMatrix, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "LocalTransform", "setMatrix(_,_)", wren_LocalTransformStaticSetMatrix, "Set the position of an entity.");

// ============================================================================
// WORLD TRANSFORM BINDINGS
// ============================================================================

// WorldTransform.getPosition() -> vec3 or null
void wren_WorldTransformGetPosition(WrenVM* vm)
{
    WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->position);
}

// WorldTransform.getPosition(entity) -> vec3 or null
void wren_WorldTransformStaticGetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->position);
}

// WorldTransform.setPosition(vec3)
void wren_WorldTransformSetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}
    
	transformSystem.SetWorldTransform(*context, transform->entity, vec->value, transform->component->scale, transform->component->rotation);
}

// WorldTransform.setPosition(entity, vec3)
void wren_WorldTransformStaticSetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}
    
    auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        DEBUG_ERROR("No Transform");
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, vec->value, transform->scale, transform->rotation);
}

// WorldTransform.getScale() -> vec3 or null
void wren_WorldTransformGetScale(WrenVM* vm)
{
    WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->scale);
}

// WorldTransform.getScale(entity) -> vec3 or null
void wren_WorldTransformStaticGetScale(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "gameObjectComponents", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->scale);
}

// WorldTransform.setScale(vec3)
void wren_WorldTransformSetScale(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}
    
	transformSystem.SetWorldTransform(*context, transform->entity, transform->component->position, vec->value, transform->component->rotation);
}

// WorldTransform.setScale(entity, vec3)
void wren_WorldTransformStaticSetScale(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
    
	if (!vec)
	{
        DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}
    
    auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        DEBUG_ERROR("No Transform");
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, transform->position, vec->value, transform->rotation);
}

// WorldTransform.getRotation() -> Quat or null
void wren_WorldTransformGetRotation(WrenVM* vm)
{
    WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "gameObjectComponents", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->component->rotation);
}

// WorldTransform.getRotation(entity) -> Quat or null
void wren_WorldTransformStaticGetRotation(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	wrenGetVariable(vm, "gameObjectComponents", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->rotation);
}

// WorldTransform.setRotation(Quat)
void wren_WorldTransformSetRotation(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));
    
	if (!quat)
	{
        DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}
    
	transformSystem.SetWorldTransform(*context, transform->entity, transform->component->position, transform->component->scale, quat->value);
}

// WorldTransform.setPosition(entity, Quat)
void wren_WorldTransformStaticSetRotation(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));
    
	if (!quat)
	{
        DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}
    
    auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        DEBUG_ERROR("Transform.setRotation: No Transform");
		return;
	}
    
	transformSystem.SetWorldTransform(*context, entity, transform->position, transform->scale, quat->value);
}

// WorldTransform.getMatrix() -> Mat4 or null
void wren_WorldTransformGetMatrix(WrenVM* vm)
{
    WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->component->matrix);
}

// WorldTransform.getMatrix(entity) -> Mat4 or null
void wren_WorldTransformStaticGetMatrix(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
        wrenSetSlotNull(vm, 0);
		return;
	}
    
	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->matrix);
}

// WorldTransform.setMatrix(mat4)
void wren_WorldTransformSetMatrix(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
    
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));
    
	if (!mat)
	{
        DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}
    
	transformSystem.SetWorldTransform(*context, transform->entity, mat->value);
}

// WorldTransform.setMatrix(entity, mat4)
void wren_WorldTransformStaticSetMatrix(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));
    
	if (!mat)
	{
        DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}
    
	transformSystem.SetWorldTransform(*context, entity, mat->value);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "WorldTransform", wren_WorldTransformAllocate, wren_WorldTransformFinalize, "Sprite animation class wraps SpriteAnimation component");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "position", wren_WorldTransformGetPosition, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "position=(_)", wren_WorldTransformSetPosition, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "rotation", wren_WorldTransformGetRotation, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "rotation=(_)", wren_WorldTransformSetRotation, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "scale", wren_WorldTransformGetScale, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "scale=(_)", wren_WorldTransformSetScale, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "matrix", wren_WorldTransformGetMatrix, "Sets the sprites texture");
WREN_CLASS_METHOD("gameObjectComponents", "WorldTransform", "matrix=(_)", wren_WorldTransformSetMatrix, "Sets the sprites texture");

// Register static methods
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "get(_)", wren_WorldTransformGet, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "getPosition(_)", wren_WorldTransformStaticGetPosition, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "setPosition(_,_)", wren_WorldTransformStaticSetPosition, "Set the position of an entity.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "getRotation(_)", wren_WorldTransformStaticGetRotation, "Get rotation of entity as Quat.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "setRotation(_,_)", wren_WorldTransformStaticSetRotation, "Set rotation of entity from Quat.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "getScale(_)", wren_WorldTransformStaticGetScale, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "setScale(_,_)", wren_WorldTransformStaticSetScale, "Set the position of an entity.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "getMatrix(_)", wren_WorldTransformStaticGetMatrix, "Get the position of an entity. Returns vec3 or null if no transform.");
WREN_CLASS_STATIC("gameObjectComponents", "WorldTransform", "setMatrix(_,_)", wren_WorldTransformStaticSetMatrix, "Set the position of an entity.");

// ============================================================================
// SCRIPT BINDINGS
// ============================================================================

// Script.create(spriteEntity, scriptPath, className) -> number
void wren_ScriptCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* scriptPath = wrenGetSlotString(vm, 2);
	const char* className = wrenGetSlotString(vm, 3);

	registry.emplace<Struktur::Component::WrenScript>(levelEntity, scriptPath, className);
}

// Script.createArg(spriteEntity, scriptPath, className, args) -> number
void wren_ScriptCreateArg(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	Struktur::System::WrenScriptSystem& scriptSystem = systemManager.GetSystem<Struktur::System::WrenScriptSystem>();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* scriptPath = wrenGetSlotString(vm, 2);
	const char* className = wrenGetSlotString(vm, 3);
	const char* args = wrenGetSlotString(vm, 4);

	auto& script = registry.emplace<Struktur::Component::WrenScript>(levelEntity, scriptPath, className, args);

	// Initialise the script

	//if (!scriptSystem.InitialiseScript(*context, levelEntity, script))
	//{
	//    DEBUG_ERROR("Failed to create script: %s", scriptPath);
	//    return;
	//}
	//scriptSystem.CallCreate(*context, levelEntity, script);


}

// Script.isInitialised -> bool
void wren_ScriptIsInitialised(WrenVM* vm)
{    
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
        DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotBool(vm, 0, script->component->isInitialised);
}

// Script.hasError -> bool
void wren_ScriptHasError(WrenVM* vm)
{    
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
        DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotBool(vm, 0, script->component->hasError);
}

// Script.getErrorMessage -> string
void wren_ScriptGetErrorMessage(WrenVM* vm)
{    
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
        DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotString(vm, 0, script->component->errorMessage.c_str());
}

// Script.getInstance() -> classHandle or null
void wren_ScriptGetInstance(WrenVM* vm)
{    
	auto* script = static_cast<WrenWrenScript*>(wrenGetSlotForeign(vm, 0));

	if (!script->component)
	{
        DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (script->component->hasError)
	{
        DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (!script->component->isInitialised)
	{
        DEBUG_WARNING("Script.get: Entity's script is not initialised");
		wrenSetSlotNull(vm, 0);
		return;
	}
	// Get the script's instance
	if (!script->component->instanceHandle)
	{
        DEBUG_ERROR("Script.get: No instance handle");
		wrenSetSlotNull(vm, 0);
		return;
	}
    
	wrenSetSlotHandle(vm, 0, script->component->instanceHandle);
}

// Script.getInstance(entity) -> classHandle or null
void wren_ScriptStaticGetInstance(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
    
	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
    
	auto* script = registry.try_get<Struktur::Component::WrenScript>(entity);
	if (!script)
	{
        DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (script->hasError)
	{
        DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (!script->isInitialised)
	{
        DEBUG_WARNING("Script.get: Entity's script is not initialised");
		wrenSetSlotNull(vm, 0);
		return;
	}
	// Get the script's instance
	if (!script->instanceHandle)
	{
        DEBUG_ERROR("Script.get: No instance handle");
		wrenSetSlotNull(vm, 0);
		return;
	}
    
	wrenSetSlotHandle(vm, 0, script->instanceHandle);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("gameObjectComponents", "Script", wren_WrenScriptAllocate, wren_WrenScriptFinalize, "Script class wraps SpriteAnimation component");

// Register methods
WREN_CLASS_METHOD("gameObjectComponents", "Script", "getInstance()", wren_ScriptGetInstance, "Gets a script instance");
WREN_CLASS_METHOD("gameObjectComponents", "Script", "isInitialised", wren_ScriptIsInitialised, "Checks if script is initialised");
WREN_CLASS_METHOD("gameObjectComponents", "Script", "hasError", wren_ScriptHasError, "Checks if script has error");
WREN_CLASS_METHOD("gameObjectComponents", "Script", "errorMessage", wren_ScriptGetErrorMessage, "Gets Scripts error message");

WREN_CLASS_STATIC("gameObjectComponents", "Script", "create(_,_,_)", wren_ScriptCreate, "Creates the script Component.");
WREN_CLASS_STATIC("gameObjectComponents", "Script", "createArg(_,_,_,_)", wren_ScriptCreateArg, "Creates the script Component with an arg.");
WREN_CLASS_STATIC("gameObjectComponents", "Script", "get(_)", wren_WrenScriptGet, "Gets the script component");
WREN_CLASS_STATIC("gameObjectComponents", "Script", "getInstance(_)", wren_ScriptStaticGetInstance, "Gets a script instance");
