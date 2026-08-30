#include "wrenGameObjectComponents.h"

#include "ComponentListXMacro.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/CameraSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/Scripting/WrenUtil.h"
#include "Engine/Util/Color.h"
#include "Engine/World/Level.h"
#include "Engine/World/RenderLayer.h"
#include "WrenAnimation.h"
#include "WrenMath.h"
#include "WrenPhysics.h"
#include "WrenResourceManager.h"
#include "wren.hpp"

// ============================================================================
// COMPONENT ALLOCATOR BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_type, component_name_string) \
	void wren_##component_name##Allocate(WrenVM* vm)                     \
	{                                                                    \
		wrenSetSlotNewForeign(vm, 0, 0, sizeof(Wren##component_name));   \
	}
COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// COMPONENT FINALIZER BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_type, component_name_string) \
	void wren_##component_name##Finalize(void* data)                     \
	{                                                                    \
		Wren##component_name* value = (Wren##component_name*)data;       \
		value->~Wren##component_name();                                  \
	}
COMPONENT_LIST
#undef COMPONENT

// ============================================================================
// COMPONENT GET BINDINGS
// ============================================================================

#define COMPONENT(component_name, component_type, component_name_string)                              \
	void wren_##component_name##Get(WrenVM* vm)                                                       \
	{                                                                                                 \
		Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));    \
		entt::registry& registry       = context->GetRegistry();                                      \
		double entityId                = wrenGetSlotDouble(vm, 1);                                    \
		entt::entity entity            = static_cast<entt::entity>(entityId);                         \
		Struktur::Component::component_type* component =                                              \
		    registry.try_get<Struktur::Component::component_type>(entity);                            \
		if (component)                                                                                \
		{                                                                                             \
			wrenGetVariable(vm, "gameObjectComponents", component_name_string, 1);                    \
			Wren##component_name* wrenComponent =                                                     \
			    (Wren##component_name*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(Wren##component_name)); \
			new (wrenComponent) Wren##component_name(entity, component);                              \
		}                                                                                             \
		else                                                                                          \
		{                                                                                             \
			wrenSetSlotNull(vm, 0);                                                                   \
		}                                                                                             \
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
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
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
	WrenCamera* camera      = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->component->zoom = (float)wrenGetSlotDouble(vm, 1);
}

void wren_CameraGetForcePosition(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, camera->component->forcePosition);
}

void wren_CameraSetForcePosition(WrenVM* vm)
{
	WrenCamera* camera               = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->component->forcePosition = wrenGetSlotBool(vm, 1);
}

void wren_CameraGetDamping(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(camera->component->damping);
}

void wren_CameraSetDamping(WrenVM* vm)
{
	WrenCamera* camera         = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	WrenVec2* damping          = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	camera->component->damping = damping->value;
}

// Camera.worldPosToScreenPos(worldPos) -> Vec2
void wren_CameraWorldPosToScreenPos(WrenVM* vm)
{
	Struktur::GameContext* context  = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::World::Camera& camera = context->GetCamera();

	WrenVec2* worldPos  = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	glm::vec2 screenPos = camera.WorldPosToScreenPos(worldPos->value);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(screenPos);
}

// Camera.screenPosToWorldPos(worldPos) -> Vec2
void wren_CameraScreenPosToWorldPos(WrenVM* vm)
{
	Struktur::GameContext* context  = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::World::Camera& camera = context->GetCamera();

	WrenVec2* screenPos = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	glm::vec2 worldPos  = camera.ScreenPosToWorldPos(screenPos->value);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(worldPos);
}

// Camera.addCameraTrauma(worldPos)
void wren_CameraAddCameraTrauma(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	Struktur::System::CameraSystem& cameraSystem   = systemManager.GetSystem<Struktur::System::CameraSystem>();

	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	float trauma       = (float)wrenGetSlotDouble(vm, 1);
	cameraSystem.AddCameraTrauma(*context, camera->entity, trauma);
}

// Camera.addCameraTrauma(entity, trauma)
void wren_CameraStaticAddCameraTrauma(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	Struktur::System::CameraSystem& cameraSystem   = systemManager.GetSystem<Struktur::System::CameraSystem>();

	entt::entity entity = (entt::entity)wrenGetSlotDouble(vm, 1);
	float trauma        = (float)wrenGetSlotDouble(vm, 2);
	cameraSystem.AddCameraTrauma(*context, entity, trauma);
}

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

// ============================================================================
// WORLD BINDINGS
// ============================================================================

// World.loadLevelEntities(levelIndex) -> number
void wren_WorldLoadLevelEntities(WrenVM* vm)
{
	WrenWorld* world = (WrenWorld*)wrenGetSlotForeign(vm, 0);

	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	double levelDouble = wrenGetSlotDouble(vm, 1);
	int levelIndex     = static_cast<int>(levelDouble);

	entt::entity levelEntity = Struktur::World::Level::LoadLevelEntities(*context, world->entity, levelIndex);

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

	double entityId          = wrenGetSlotDouble(vm, 1);
	entt::entity worldEntity = static_cast<entt::entity>(entityId);

	double levelDouble = wrenGetSlotDouble(vm, 2);
	int levelIndex     = static_cast<int>(levelDouble);

	entt::entity levelEntity = Struktur::World::Level::LoadLevelEntities(*context, worldEntity, levelIndex);

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
	const char* worldFilePath      = wrenGetSlotString(vm, 1);
	entt::entity worldEntity       = Struktur::World::Level::CreateWorldEntity(*context, worldFilePath);
	double entityId                = static_cast<double>(worldEntity);
	wrenSetSlotDouble(vm, 0, entityId);
}

// World.getLevelsCount() -> number
void wren_WorldGetLevelsCount(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	WrenWorld* world      = (WrenWorld*)wrenGetSlotForeign(vm, 0);

	Struktur::FileLoading::LevelParser::World& worldMap = world->component->worldMap;

	double doubleNumber = static_cast<double>(worldMap.levels.size());

	wrenSetSlotDouble(vm, 0, doubleNumber);
}

// World.getLevelsCount(entity) -> number
void wren_WorldStaticGetLevelsCount(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId       = wrenGetSlotDouble(vm, 1);
	entt::entity entity   = static_cast<entt::entity>(entityId);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);

	if (!worldComponent)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	Struktur::FileLoading::LevelParser::World& worldMap = worldComponent->worldMap;

	double doubleNumber = static_cast<double>(worldMap.levels.size());

	wrenSetSlotDouble(vm, 0, doubleNumber);
}

// World.getLevelIndex(levelName) -> number
void wren_WorldGetLevelIndex(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	WrenWorld* world      = (WrenWorld*)wrenGetSlotForeign(vm, 0);
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
	entt::registry& registry       = context->GetRegistry();

	double entityId       = wrenGetSlotDouble(vm, 1);
	const char* levelName = wrenGetSlotString(vm, 2);
	entt::entity entity   = static_cast<entt::entity>(entityId);

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

// ============================================================================
// PHYSICS BODY BINDINGS
// ============================================================================

// PhysicsBody.create(entity, bodyDef, shape) -> PhysicsBody
void wren_PhysicsBodyCreate(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry                       = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem                            = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	double entityId             = wrenGetSlotDouble(vm, 1);
	entt::entity entity         = static_cast<entt::entity>(entityId);
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 2));
	// This can possibly cause issues if the underlying bs shape does not have a b2shape as its fist attribute
	b2Shape* shape = static_cast<b2Shape*>(wrenGetSlotForeign(vm, 3));

	Struktur::Component::PhysicsBody& physicsBodyComponent =
	    physicsSystem.CreatePhysicsBody(*context, entity, bodyDef->bodyDef, *shape);

	wrenGetVariable(vm, "gameObjectComponents", "PhysicsBody", 1);  // Get class into slot 1
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenPhysicsBody));

	// PhysicsBody.new() - identity
	new (physicsBody) WrenPhysicsBody(entity, &physicsBodyComponent);
}

void wren_PhysicsBodySetFixedRotation(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	bool fixedRotation           = (float)wrenGetSlotBool(vm, 1);
	physicsBody->component->body->SetFixedRotation(fixedRotation);
}

// PhysicsBody.setLinearVelocity(entity, velocity)
void wren_PhysicsBodyStaticSetLinearVelocity(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                                 = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenVec2* velocity  = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

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

// PhysicsBody.setCollisionFilter(categoryBits, maskBits) - see physics.CollisionLayers for allocating the bits
void wren_PhysicsBodySetCollisionFilter(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem                            = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	uint16_t categoryBits        = static_cast<uint16_t>(wrenGetSlotDouble(vm, 1));
	uint16_t maskBits            = static_cast<uint16_t>(wrenGetSlotDouble(vm, 2));

	physicsSystem.SetCollisionFilter(*physicsBody->component, categoryBits, maskBits);
}

// ============================================================================
// SHADER BINDINGS
// ============================================================================

// Shader.create(entity, shaderResourse) -> Shader
void wren_ShaderCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId                  = wrenGetSlotDouble(vm, 1);
	entt::entity entity              = static_cast<entt::entity>(entityId);
	WrenShaderHandle* shaderResource = static_cast<WrenShaderHandle*>(wrenGetSlotForeign(vm, 2));

	auto& shaderComponent = registry.emplace<Struktur::Component::Shader>(entity, shaderResource->resource);

	wrenGetVariable(vm, "gameObjectComponents", "Shader", 1);  // Get class into slot 1
	WrenShader* shader = static_cast<WrenShader*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenShader)));

	// Shader.new() - identity
	new (shader) WrenShader(entity, &shaderComponent);
}

void wren_ShaderSetFloatUniform(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& shaderSystem                             = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = static_cast<WrenShader*>(wrenGetSlotForeign(vm, 0));
	const char* name   = wrenGetSlotString(vm, 1);
	float value        = static_cast<float>(wrenGetSlotDouble(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value);
}

void wren_ShaderSetIntUniform(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& shaderSystem                             = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = static_cast<WrenShader*>(wrenGetSlotForeign(vm, 0));
	const char* name   = wrenGetSlotString(vm, 1);
	int value          = static_cast<int>(wrenGetSlotDouble(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value);
}

void wren_ShaderSetVec2Uniform(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& shaderSystem                             = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = static_cast<WrenShader*>(wrenGetSlotForeign(vm, 0));
	const char* name   = wrenGetSlotString(vm, 1);
	WrenVec2* value    = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

void wren_ShaderSetVec3Uniform(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& shaderSystem                             = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = static_cast<WrenShader*>(wrenGetSlotForeign(vm, 0));
	const char* name   = wrenGetSlotString(vm, 1);
	WrenVec3* value    = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

void wren_ShaderSetVec4Uniform(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& shaderSystem                             = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = static_cast<WrenShader*>(wrenGetSlotForeign(vm, 0));
	const char* name   = wrenGetSlotString(vm, 1);
	WrenVec4* value    = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

void wren_ShaderSetMat4Uniform(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& shaderSystem                             = systemManager.GetSystem<Struktur::System::ShaderSystem>();

	WrenShader* shader = static_cast<WrenShader*>(wrenGetSlotForeign(vm, 0));
	const char* name   = wrenGetSlotString(vm, 1);
	WrenMat4* value    = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 2));
	shaderSystem.SetUniform(*context, shader->entity, name, value->value);
}

// ============================================================================
// SPRITE ANIMATION BINDINGS
// ============================================================================

// SpriteAnimation.create(entity) -> SpriteAnimation
void wren_SpriteAnimationCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	Struktur::Component::SpriteAnimation& spriteAnimationComponent =
	    registry.emplace<Struktur::Component::SpriteAnimation>(entity);

	wrenGetVariable(vm, "gameObjectComponents", "SpriteAnimation", 1);  // Get class into slot 1
	WrenSpriteAnimation* spriteAnimation =
	    static_cast<WrenSpriteAnimation*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenSpriteAnimation)));

	// SpriteAnimation.new() - identity
	new (spriteAnimation) WrenSpriteAnimation(entity, &spriteAnimationComponent);
}

// SpriteAnimation.addAnimation(animationKey, animationDefinition)
void wren_SpriteAnimationAddAnimation(WrenVM* vm)
{
	Struktur::GameContext* context                     = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager     = context->GetSystemManager();
	Struktur::System::AnimationSystem& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();

	auto* spriteAnimation     = static_cast<WrenSpriteAnimation*>(wrenGetSlotForeign(vm, 0));
	const char* animationKey  = wrenGetSlotString(vm, 1);
	auto* animationDefinition = static_cast<WrenSpriteAnimationDefinition*>(wrenGetSlotForeign(vm, 2));

	animationSystem.AddAnimation(*context, spriteAnimation->entity, animationKey, animationDefinition->spriteAnimation);
}

// SpriteAnimation.setCurrentAnimation(entity, animationName)
void wren_SpriteAnimationStaticSetCurrentAnimation(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem                          = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry                                 = context->GetRegistry();

	double entityId           = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity       = static_cast<entt::entity>(entityId);

	if (!animationSystem.IsAnimationPlaying(*context, entity, animationName))
	{
		animationSystem.PlayAnimation(*context, entity, animationName);
	}
}

// SpriteAnimation.playAnimation(entity, animationName)
void wren_SpriteAnimationPlayAnimation(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem                          = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry                                 = context->GetRegistry();

	double entityId           = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity       = static_cast<entt::entity>(entityId);

	animationSystem.PlayAnimation(*context, entity, animationName);
}

// SpriteAnimation.isAnimationPlaying(entity, animationName) -> bool
void wren_SpriteAnimationIsAnimationPlaying(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem                          = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry                                 = context->GetRegistry();

	double entityId           = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity       = static_cast<entt::entity>(entityId);

	bool isAnimationPlaying = animationSystem.IsAnimationPlaying(*context, entity, animationName);
	wrenSetSlotBool(vm, 0, isAnimationPlaying);
}

// ============================================================================
// PARTICLE EMITTER BINDINGS
// ============================================================================

// ParticleEmitter.create(entity, texture) -> ParticleEmitter
void wren_ParticleEmitterCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	entt::entity entity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));

	// Same guard as Sprite.create - texture is null when Texture.load() failed, and wrenGetSlotForeign on a
	// null slot returns a garbage pointer rather than nullptr.
	if (wrenGetSlotType(vm, 2) != WREN_TYPE_FOREIGN)
	{
		wrenSetSlotString(vm, 0, "ParticleEmitter.create: texture is not a valid Texture (did Texture.load() fail?)");
		wrenAbortFiber(vm, 0);
		return;
	}
	WrenTextureHandle* texture = static_cast<WrenTextureHandle*>(wrenGetSlotForeign(vm, 2));

	auto& emitterComponent  = registry.emplace<Struktur::Component::ParticleEmitter>(entity);
	emitterComponent.texture = texture->resource;

	wrenGetVariable(vm, "gameObjectComponents", "ParticleEmitter", 1);  // Get class into slot 1
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenParticleEmitter));
	new (emitter) WrenParticleEmitter(entity, &emitterComponent);
}

void wren_ParticleEmitterSetTexture(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenTextureHandle* texture   = (WrenTextureHandle*)wrenGetSlotForeign(vm, 1);
	emitter->component->texture  = texture->resource;
}

void wren_ParticleEmitterSetColumns(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->columns  = static_cast<int>(wrenGetSlotDouble(vm, 1));
}

void wren_ParticleEmitterGetColumns(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->columns);
}

void wren_ParticleEmitterSetRows(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->rows     = static_cast<int>(wrenGetSlotDouble(vm, 1));
}

void wren_ParticleEmitterGetRows(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->rows);
}

void wren_ParticleEmitterSetEmissionRate(WrenVM* vm)
{
	WrenParticleEmitter* emitter      = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->emissionRate = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetEmissionRate(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->emissionRate);
}

void wren_ParticleEmitterSetBurstCount(WrenVM* vm)
{
	WrenParticleEmitter* emitter    = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->burstCount = static_cast<int>(wrenGetSlotDouble(vm, 1));
	// A new burst count should fire again - matches the intuitive "set it and it happens" script usage,
	// rather than requiring the emitter to be destroyed/recreated to trigger a second burst.
	emitter->component->hasBurst = false;
}

void wren_ParticleEmitterGetBurstCount(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->burstCount);
}

void wren_ParticleEmitterSetSpawnRadius(WrenVM* vm)
{
	WrenParticleEmitter* emitter     = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->spawnRadius = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetSpawnRadius(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->spawnRadius);
}

void wren_ParticleEmitterSetVelocityMin(WrenVM* vm)
{
	WrenParticleEmitter* emitter    = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenVec2* velocity               = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	emitter->component->velocityMin = velocity->value;
}

void wren_ParticleEmitterGetVelocityMin(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(emitter->component->velocityMin);
}

void wren_ParticleEmitterSetVelocityMax(WrenVM* vm)
{
	WrenParticleEmitter* emitter    = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenVec2* velocity               = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	emitter->component->velocityMax = velocity->value;
}

void wren_ParticleEmitterGetVelocityMax(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(emitter->component->velocityMax);
}

void wren_ParticleEmitterSetAcceleration(WrenVM* vm)
{
	WrenParticleEmitter* emitter      = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenVec2* acceleration             = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	emitter->component->acceleration = acceleration->value;
}

void wren_ParticleEmitterGetAcceleration(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(emitter->component->acceleration);
}

void wren_ParticleEmitterSetLifetimeMin(WrenVM* vm)
{
	WrenParticleEmitter* emitter    = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->lifetimeMin = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetLifetimeMin(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->lifetimeMin);
}

void wren_ParticleEmitterSetLifetimeMax(WrenVM* vm)
{
	WrenParticleEmitter* emitter    = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->lifetimeMax = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetLifetimeMax(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->lifetimeMax);
}

void wren_ParticleEmitterSetStartColor(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenVec4* color               = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	emitter->component->startColor = Struktur::Util::Color{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                                                       (unsigned char)color->value.b, (unsigned char)color->value.a};
}

void wren_ParticleEmitterGetStartColor(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "gameObjectComponents", "ParticleEmitter", 1);  // Get class into slot 1
	WrenVec4* color = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (color) WrenVec4(glm::vec4(emitter->component->startColor));
}

void wren_ParticleEmitterSetEndColor(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenVec4* color               = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	emitter->component->endColor = Struktur::Util::Color{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                                                     (unsigned char)color->value.b, (unsigned char)color->value.a};
}

void wren_ParticleEmitterGetEndColor(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "gameObjectComponents", "ParticleEmitter", 1);  // Get class into slot 1
	WrenVec4* color = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (color) WrenVec4(glm::vec4(emitter->component->endColor));
}

void wren_ParticleEmitterSetStartScale(WrenVM* vm)
{
	WrenParticleEmitter* emitter   = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->startScale = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetStartScale(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->startScale);
}

void wren_ParticleEmitterSetEndScale(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->endScale = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetEndScale(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->endScale);
}

void wren_ParticleEmitterSetRotationSpeedMin(WrenVM* vm)
{
	WrenParticleEmitter* emitter          = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->rotationSpeedMin = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetRotationSpeedMin(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->rotationSpeedMin);
}

void wren_ParticleEmitterSetRotationSpeedMax(WrenVM* vm)
{
	WrenParticleEmitter* emitter          = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->rotationSpeedMax = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetRotationSpeedMax(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->rotationSpeedMax);
}

void wren_ParticleEmitterSetAdditive(WrenVM* vm)
{
	WrenParticleEmitter* emitter  = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->additive = wrenGetSlotBool(vm, 1);
}

void wren_ParticleEmitterGetAdditive(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, emitter->component->additive);
}

void wren_ParticleEmitterSetLayer(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	int layer                     = static_cast<int>(wrenGetSlotDouble(vm, 1));
	emitter->component->layer    = static_cast<Struktur::World::RenderLayer>(layer);
}

void wren_ParticleEmitterGetLayer(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->layer);
}

void wren_ParticleEmitterSetOrderInLayer(WrenVM* vm)
{
	WrenParticleEmitter* emitter      = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->orderInLayer = (float)wrenGetSlotDouble(vm, 1);
}

void wren_ParticleEmitterGetOrderInLayer(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->orderInLayer);
}

void wren_ParticleEmitterSetMaxParticles(WrenVM* vm)
{
	WrenParticleEmitter* emitter     = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->maxParticles = static_cast<int>(wrenGetSlotDouble(vm, 1));
}

void wren_ParticleEmitterGetMaxParticles(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, (double)emitter->component->maxParticles);
}

void wren_ParticleEmitterSetLooping(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->looping  = wrenGetSlotBool(vm, 1);
}

void wren_ParticleEmitterGetLooping(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, emitter->component->looping);
}

// ParticleEmitter.aliveCount -> Num - current live particle count, mostly useful for a script deciding when a
// finished (looping=false, burst spent, no particles left alive) one-shot effect's entity can be torn down.
void wren_ParticleEmitterGetAliveCount(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	int aliveCount                = 0;
	for (const auto& particle : emitter->component->particles)
	{
		if (particle.alive)
		{
			++aliveCount;
		}
	}
	wrenSetSlotDouble(vm, 0, (double)aliveCount);
}

// ============================================================================
// SPRITE BINDINGS
// ============================================================================

// Sprite.create(spriteEntity, texture, color, offset, columns, rows, flipped, index, layer, orderInLayer) -> number
void wren_SpriteCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));

	// texture is null when Texture.load() failed (e.g. missing/renamed asset) - wrenGetSlotForeign on a null
	// slot returns a garbage pointer, not nullptr, so this must be checked before it's ever dereferenced below
	// (previously this crashed the whole process instead of the load failure staying a recoverable Wren error).
	if (wrenGetSlotType(vm, 2) != WREN_TYPE_FOREIGN)
	{
		wrenSetSlotString(vm, 0, "Sprite.create: texture is not a valid Texture (did Texture.load() fail?)");
		wrenAbortFiber(vm, 0);
		return;
	}

	WrenTextureHandle* texture = static_cast<WrenTextureHandle*>(wrenGetSlotForeign(vm, 2));
	WrenVec4* color            = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* offset           = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	int columns                = static_cast<int>(wrenGetSlotDouble(vm, 5));
	int rows                   = static_cast<int>(wrenGetSlotDouble(vm, 6));
	bool flipped               = wrenGetSlotBool(vm, 7);
	int index                  = static_cast<int>(wrenGetSlotDouble(vm, 8));
	auto layer                 = static_cast<Struktur::World::RenderLayer>(static_cast<int>(wrenGetSlotDouble(vm, 9)));
	float orderInLayer         = static_cast<float>(wrenGetSlotDouble(vm, 10));

	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};

	registry.emplace<Struktur::Component::Sprite>(levelEntity, texture->resource, rayColor, offset->value, columns,
	                                              rows, flipped, index, layer, orderInLayer);
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
	WrenSprite* sprite         = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	WrenTextureHandle* texture = (WrenTextureHandle*)wrenGetSlotForeign(vm, 1);
	sprite->component->texture = texture->resource;
}

void wren_SpriteGetColor(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
	WrenVec4* color = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	glm::vec4 glmColor((float)sprite->component->color.r, (float)sprite->component->color.g,
	                   (float)sprite->component->color.b, (float)sprite->component->color.a);
	new (color) WrenVec4(glmColor);
}

void wren_SpriteSetColor(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	WrenVec4* color    = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	Struktur::Util::Color rayColor{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                               (unsigned char)color->value.b, (unsigned char)color->value.a};
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
	WrenSprite* sprite        = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	WrenVec2* offset          = (WrenVec2*)wrenGetSlotForeign(vm, 1);
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
	WrenSprite* sprite         = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int columns                = static_cast<int>(wrenGetSlotDouble(vm, 1));
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
	WrenSprite* sprite      = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int rows                = static_cast<int>(wrenGetSlotDouble(vm, 1));
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
	WrenSprite* sprite       = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int index                = static_cast<int>(wrenGetSlotDouble(vm, 1));
	sprite->component->index = index;
}

void wren_SpriteGetLayer(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
	double layer = static_cast<double>(sprite->component->layer);
	wrenSetSlotDouble(vm, 0, layer);
}

void wren_SpriteSetLayer(WrenVM* vm)
{
	WrenSprite* sprite       = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	int layer                = static_cast<int>(wrenGetSlotDouble(vm, 1));
	sprite->component->layer = static_cast<Struktur::World::RenderLayer>(layer);
}

void wren_SpriteGetOrderInLayer(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	wrenGetVariable(vm, "gameObjectComponents", "Sprite", 1);  // Get class into slot 1
	double orderInLayer = static_cast<double>(sprite->component->orderInLayer);
	wrenSetSlotDouble(vm, 0, orderInLayer);
}

void wren_SpriteSetOrderInLayer(WrenVM* vm)
{
	WrenSprite* sprite              = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	float orderInLayer              = static_cast<float>(wrenGetSlotDouble(vm, 1));
	sprite->component->orderInLayer = orderInLayer;
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
	WrenSprite* sprite         = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	bool flipped               = wrenGetSlotBool(vm, 1);
	sprite->component->flipped = flipped;
}

// Sprite.setLayer(entity, layer)
void wren_SpriteStaticSetLayer(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	double layer        = wrenGetSlotDouble(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->layer = static_cast<Struktur::World::RenderLayer>(static_cast<int>(layer));
}

// Sprite.setOrderInLayer(entity, orderInLayer)
void wren_SpriteStaticSetOrderInLayer(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	double orderInLayer = wrenGetSlotDouble(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->orderInLayer = static_cast<float>(orderInLayer);
}

// Sprite.setFlipped(entity, flipped)
void wren_SpriteStaticSetFlipped(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	bool flipped        = wrenGetSlotBool(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->flipped = flipped;
}

// ============================================================================
// LOCAL TRANSFORM BINDINGS
// ============================================================================

// LocalTransform.getPosition() -> vec3 or null
void wren_LocalTransformGetPosition(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->localPosition);
}

// LocalTransform.getPosition(entity) -> vec3 or null
void wren_LocalTransformStaticGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->localPosition);
}

// LocalTransform.setPosition(vec3)
void wren_LocalTransformSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, vec->value, transform->component->localScale,
	                                  transform->component->localRotation);
}

// LocalTransform.setPosition(entity, vec3)
void wren_LocalTransformStaticSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, vec->value, glm::vec3(1.0f),
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, vec->value, transform->localScale, transform->localRotation);
}

// LocalTransform.getScale() -> vec3 or null
void wren_LocalTransformGetScale(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->component->localScale);
}

// LocalTransform.getScale(entity) -> vec3 or null
void wren_LocalTransformStaticGetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->localScale);
}

// LocalTransform.setScale(vec3)
void wren_LocalTransformSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, transform->component->localPosition, vec->value,
	                                  transform->component->localRotation);
}

// LocalTransform.setScale(entity, vec3)
void wren_LocalTransformStaticSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), vec->value,
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, transform->localPosition, vec->value, transform->localRotation);
}

// LocalTransform.getRotation() -> Quat or null
void wren_LocalTransformGetRotation(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->component->localRotation);
}

// LocalTransform.getRotation(entity) -> Quat or null
void wren_LocalTransformStaticGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->localRotation);
}

// LocalTransform.setRotation(Quat)
void wren_LocalTransformSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	transformSystem.SetLocalTransform(*context, transform->entity, transform->component->localPosition,
	                                  transform->component->localScale, quat->value);
}

// LocalTransform.setPosition(entity, Quat)
void wren_LocalTransformStaticSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), glm::vec3(1.0f), quat->value);
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, transform->localPosition, transform->localScale, quat->value);
}

// LocalTransform.getMatrix() -> Mat4 or null
void wren_LocalTransformGetMatrix(WrenVM* vm)
{
	WrenLocalTransform* transform = (WrenLocalTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->component->localMatrix);
}

// LocalTransform.getMatrix(entity) -> Mat4 or null
void wren_LocalTransformStaticGetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::Transform>(entity);
	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "gameObjectComponents", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transform->localMatrix);
}

// LocalTransform.setMatrix(mat4)
void wren_LocalTransformSetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

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
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	if (!mat)
	{
		DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, mat->value);
}

// ============================================================================
// WORLD TRANSFORM BINDINGS
// ============================================================================

// WorldTransform.getPosition() -> vec3 or null
void wren_WorldTransformGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldPosition(*context, transform->entity));
}

// WorldTransform.getPosition(entity) -> vec3 or null
void wren_WorldTransformStaticGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldPosition(*context, entity));
}

// WorldTransform.setPosition(vec3)
void wren_WorldTransformSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity, vec->value,
	                                  transformSystem.GetWorldScale(*context, transform->entity),
	                                  transformSystem.GetWorldRotation(*context, transform->entity));
}

// WorldTransform.setPosition(entity, vec3)
void wren_WorldTransformStaticSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, vec->value, glm::vec3(1.0f),
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, vec->value, transformSystem.GetWorldScale(*context, entity),
	                                  transformSystem.GetWorldRotation(*context, entity));
}

// WorldTransform.getScale() -> vec3 or null
void wren_WorldTransformGetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldScale(*context, transform->entity));
}

// WorldTransform.getScale(entity) -> vec3 or null
void wren_WorldTransformStaticGetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with scale
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transformSystem.GetWorldScale(*context, entity));
}

// WorldTransform.setScale(vec3)
void wren_WorldTransformSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity,
	                                  transformSystem.GetWorldPosition(*context, transform->entity), vec->value,
	                                  transformSystem.GetWorldRotation(*context, transform->entity));
}

// WorldTransform.setScale(entity, vec3)
void wren_WorldTransformStaticSetScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setScale: Invalid Vec3");
		return;
	}

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), vec->value,
		                                  glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, transformSystem.GetWorldPosition(*context, entity), vec->value,
	                                  transformSystem.GetWorldRotation(*context, entity));
}

// WorldTransform.getRotation() -> Quat or null
void wren_WorldTransformGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transformSystem.GetWorldRotation(*context, transform->entity));
}

// WorldTransform.getRotation(entity) -> Quat or null
void wren_WorldTransformStaticGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transformSystem.GetWorldRotation(*context, entity));
}

// WorldTransform.setRotation(Quat)
void wren_WorldTransformSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	WrenWorldTransform* transform = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	transformSystem.SetWorldTransform(*context, transform->entity,
	                                  transformSystem.GetWorldPosition(*context, transform->entity),
	                                  transformSystem.GetWorldScale(*context, transform->entity), quat->value);
}

// WorldTransform.setPosition(entity, Quat)
void wren_WorldTransformStaticSetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 1));

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		// should always be a world transform so create one here
		transformSystem.SetWorldTransform(*context, entity, glm::vec3(0.0f), glm::vec3(1.0f), quat->value);
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, transformSystem.GetWorldPosition(*context, entity),
	                                  transformSystem.GetWorldScale(*context, entity), quat->value);
}

// WorldTransform.getMatrix() -> Mat4 or null
void wren_WorldTransformGetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();
	WrenWorldTransform* transform  = (WrenWorldTransform*)wrenGetSlotForeign(vm, 0);
	// Create Vec3 foreign object with rotation
	wrenGetVariable(vm, "math", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transformSystem.GetWorldMatrix(*context, transform->entity));
}

// WorldTransform.getMatrix(entity) -> Mat4 or null
void wren_WorldTransformStaticGetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry                 = context->GetRegistry();
	auto& transformSystem          = context->GetSystemManager().GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!registry.try_get<Struktur::Component::Transform>(entity))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Mat4", 1);  // Get class into slot 1
	WrenMat4* mat = (WrenMat4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4));
	new (mat) WrenMat4(transformSystem.GetWorldMatrix(*context, entity));
}

// WorldTransform.setMatrix(mat4)
void wren_WorldTransformSetMatrix(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

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
	auto& registry                 = context->GetRegistry();
	auto& systemManager            = context->GetSystemManager();
	auto& transformSystem          = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	if (!mat)
	{
		DEBUG_ERROR("Transform.setmatrix: Invalid Mat4");
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, mat->value);
}

// ============================================================================
// SCRIPT BINDINGS
// ============================================================================

// Script.create(entity, className) -> ScriptInstance
void wren_ScriptCreate(WrenVM* vm)
{
	entt::registry& registry = static_cast<Struktur::GameContext*>(wrenGetUserData(vm))->GetRegistry();

	entt::entity entity   = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* className = wrenGetSlotString(vm, 2);

	// GameObjectManager::OnScriptConstruct initialises the script and queues its Start() call.
	registry.emplace<Struktur::Component::WrenScript>(entity, className);
}

// Script.createArgPairs(entity, className, pairs) -> ScriptInstance
// Internal: `pairs` is the MapUtil.mapToPairs encoding of the arg map. Wren has no notion of a
// private method, so this is just a plain method - prefer the public Script.createArg(entity,
// className, map) Wren-side wrapper below, which calls through to this.
void wren_ScriptCreateArgPairs(WrenVM* vm)
{
	entt::registry& registry = static_cast<Struktur::GameContext*>(wrenGetUserData(vm))->GetRegistry();

	entt::entity entity   = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* className = wrenGetSlotString(vm, 2);
	wrenEnsureSlots(vm, 6);
	std::vector<Struktur::Wren::WrenItem> wrenArgs = Struktur::Wren::Util::GetWrenMapFromPairs(vm, 3, 4);

	// GameObjectManager::OnScriptConstruct initialises the script and queues its Start() call.
	registry.emplace<Struktur::Component::WrenScript>(entity, className, wrenArgs);
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
		// DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
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
	auto& registry                 = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
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
		// DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
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

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(GameObjectComponent)
{
	WREN_ENUM(registry, "gameObjectComponents", RenderLayer, "Coarse draw-order buckets for sprites and tile layers",
	          WREN_ENUM_PAIR("BACKGROUND_FAR", Struktur::World::RenderLayer::BackgroundFar),
	          WREN_ENUM_PAIR("BACKGROUND_MID", Struktur::World::RenderLayer::BackgroundMid),
	          WREN_ENUM_PAIR("ENTITIES", Struktur::World::RenderLayer::Entities),
	          WREN_ENUM_PAIR("BACKGROUND_OVERLAY", Struktur::World::RenderLayer::BackgroundOverlay),
	          WREN_ENUM_PAIR("FOREGROUND", Struktur::World::RenderLayer::Foreground),
	          WREN_ENUM_PAIR("UI", Struktur::World::RenderLayer::UI), );

	// Register ParticleEmitter Component foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "ParticleEmitter", wren_ParticleEmitterAllocate,
	                   wren_ParticleEmitterFinalize, "ParticleEmitter component class");

	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "texture=(_)", wren_ParticleEmitterSetTexture,
	                  "Set the emitted particles' texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "columns", wren_ParticleEmitterGetColumns,
	                  "Get the texture's atlas column count");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "columns=(_)", wren_ParticleEmitterSetColumns,
	                  "Set the texture's atlas column count");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "rows", wren_ParticleEmitterGetRows,
	                  "Get the texture's atlas row count");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "rows=(_)", wren_ParticleEmitterSetRows,
	                  "Set the texture's atlas row count");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "emissionRate",
	                  wren_ParticleEmitterGetEmissionRate, "Get particles spawned per second while looping");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "emissionRate=(_)",
	                  wren_ParticleEmitterSetEmissionRate, "Set particles spawned per second while looping");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "burstCount", wren_ParticleEmitterGetBurstCount,
	                  "Get the one-shot particle count spawned once when the emitter becomes active");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "burstCount=(_)",
	                  wren_ParticleEmitterSetBurstCount,
	                  "Set the one-shot particle count - setting this fires a new burst");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "spawnRadius",
	                  wren_ParticleEmitterGetSpawnRadius, "Get the spawn area radius (0 = point emitter)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "spawnRadius=(_)",
	                  wren_ParticleEmitterSetSpawnRadius, "Set the spawn area radius (0 = point emitter)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "velocityMin",
	                  wren_ParticleEmitterGetVelocityMin, "Get the minimum per-axis spawn velocity");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "velocityMin=(_)",
	                  wren_ParticleEmitterSetVelocityMin, "Set the minimum per-axis spawn velocity");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "velocityMax",
	                  wren_ParticleEmitterGetVelocityMax, "Get the maximum per-axis spawn velocity");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "velocityMax=(_)",
	                  wren_ParticleEmitterSetVelocityMax, "Set the maximum per-axis spawn velocity");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "acceleration",
	                  wren_ParticleEmitterGetAcceleration, "Get the constant per-particle acceleration (e.g. gravity)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "acceleration=(_)",
	                  wren_ParticleEmitterSetAcceleration, "Set the constant per-particle acceleration (e.g. gravity)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "lifetimeMin",
	                  wren_ParticleEmitterGetLifetimeMin, "Get the minimum particle lifetime in seconds");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "lifetimeMin=(_)",
	                  wren_ParticleEmitterSetLifetimeMin, "Set the minimum particle lifetime in seconds");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "lifetimeMax",
	                  wren_ParticleEmitterGetLifetimeMax, "Get the maximum particle lifetime in seconds");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "lifetimeMax=(_)",
	                  wren_ParticleEmitterSetLifetimeMax, "Set the maximum particle lifetime in seconds");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "startColor",
	                  wren_ParticleEmitterGetStartColor, "Get the color a particle spawns with");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "startColor=(_)",
	                  wren_ParticleEmitterSetStartColor, "Set the color a particle spawns with");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "endColor", wren_ParticleEmitterGetEndColor,
	                  "Get the color a particle lerps to over its lifetime");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "endColor=(_)", wren_ParticleEmitterSetEndColor,
	                  "Set the color a particle lerps to over its lifetime");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "startScale",
	                  wren_ParticleEmitterGetStartScale, "Get the scale a particle spawns with");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "startScale=(_)",
	                  wren_ParticleEmitterSetStartScale, "Set the scale a particle spawns with");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "endScale", wren_ParticleEmitterGetEndScale,
	                  "Get the scale a particle lerps to over its lifetime");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "endScale=(_)", wren_ParticleEmitterSetEndScale,
	                  "Set the scale a particle lerps to over its lifetime");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "rotationSpeedMin",
	                  wren_ParticleEmitterGetRotationSpeedMin, "Get the minimum spawn rotation speed (radians/sec)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "rotationSpeedMin=(_)",
	                  wren_ParticleEmitterSetRotationSpeedMin, "Set the minimum spawn rotation speed (radians/sec)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "rotationSpeedMax",
	                  wren_ParticleEmitterGetRotationSpeedMax, "Get the maximum spawn rotation speed (radians/sec)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "rotationSpeedMax=(_)",
	                  wren_ParticleEmitterSetRotationSpeedMax, "Set the maximum spawn rotation speed (radians/sec)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "additive", wren_ParticleEmitterGetAdditive,
	                  "Get whether particles blend additively instead of with normal alpha blending");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "additive=(_)", wren_ParticleEmitterSetAdditive,
	                  "Set whether particles blend additively instead of with normal alpha blending");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "layer", wren_ParticleEmitterGetLayer,
	                  "Get the render layer particles draw on");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "layer=(_)", wren_ParticleEmitterSetLayer,
	                  "Set the render layer particles draw on");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "orderInLayer",
	                  wren_ParticleEmitterGetOrderInLayer, "Get the draw order within the render layer");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "orderInLayer=(_)",
	                  wren_ParticleEmitterSetOrderInLayer, "Set the draw order within the render layer");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "maxParticles",
	                  wren_ParticleEmitterGetMaxParticles, "Get the particle pool capacity");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "maxParticles=(_)",
	                  wren_ParticleEmitterSetMaxParticles, "Set the particle pool capacity");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "looping", wren_ParticleEmitterGetLooping,
	                  "Get whether the emitter continuously spawns particles at emissionRate");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "looping=(_)", wren_ParticleEmitterSetLooping,
	                  "Set whether the emitter continuously spawns particles at emissionRate");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "aliveCount",
	                  wren_ParticleEmitterGetAliveCount, "Get the current live particle count");

	WREN_CLASS_STATIC(registry, "gameObjectComponents", "ParticleEmitter", "create(_,_)", wren_ParticleEmitterCreate,
	                  "Creates a particle emitter component with the given texture.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "ParticleEmitter", "get(_)", wren_ParticleEmitterGet,
	                  "Gets a particle emitter component.");

	// Register Camera Component foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "Camera", wren_CameraAllocate, wren_CameraFinalize,
	                   "Camera component class");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "zoom", wren_CameraGetZoom, "Get the zoom");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "zoom=(_)", wren_CameraSetZoom, "Set the zoom");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "forcePosition", wren_CameraGetForcePosition,
	                  "Get the forcePosition, will directly set the position of the next frame");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "forcePosition=(_)", wren_CameraSetForcePosition,
	                  "Set the forcePosition, will directly set the position of the next frame");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "damping", wren_CameraGetDamping, "Get the damping");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "damping=(_)", wren_CameraSetDamping,
	                  "Set the damping");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "addCameraTrauma(_)", wren_CameraAddCameraTrauma,
	                  "Add Trauma to camera for screen shake");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Camera", "create(_)", wren_CameraCreate,
	                  "Creates a camera component.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Camera", "get(_)", wren_CameraGet, "Gets a camera component.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Camera", "worldPosToScreenPos(_)",
	                  wren_CameraWorldPosToScreenPos,
	                  "Converts a world position to the screen position from the currently active camera.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Camera", "screenPosToWorldPos(_)",
	                  wren_CameraScreenPosToWorldPos,
	                  "Converts a screen position to the world position from the currently active camera.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Camera", "addCameraTrauma(_,_)",
	                  wren_CameraStaticAddCameraTrauma, "Add Trauma to camera for screen shake active camera.");

	// Register Level Component foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "Level", wren_LevelAllocate, wren_LevelFinalize,
	                   "Level component class");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Level", "index", wren_LevelGetIndex, "Get the level index");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Level", "width", wren_LevelGetWidth, "Get the level width");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Level", "height", wren_LevelGetHeight, "Get the level height");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Level", "get(_)", wren_LevelGet, "Gets a level component.");

	// Register Level Component foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "World", wren_WorldAllocate, wren_WorldFinalize,
	                   "World component class");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "loadLevelEntities(_)", wren_WorldLoadLevelEntities,
	                  "Creates a level in the game and all its corresponding objects and entities.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLevelIndex(_)", wren_WorldGetLevelIndex,
	                  "Get the index of an Level in the world.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLevelsCount()", wren_WorldGetLevelsCount,
	                  "Gets the number of levels in the world.");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "get(_)", wren_WorldGet, "Gets a world component.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "loadLevelEntities(_,_)",
	                  wren_WorldStaticLoadLevelEntities,
	                  "Creates a level in the game and all its corresponding objects and entities.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "createWorldEntity(_)", wren_WorldCreateWorldEntity,
	                  "Loads in a LDTK world file and creates the world game object and corresponding components.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelIndex(_,_)", wren_WorldStaticGetLevelIndex,
	                  "Get the index of an Level in the world.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelsCount(_)", wren_WorldStaticGetLevelsCount,
	                  "Gets the number of levels in the world.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "PhysicsBody", wren_PhysicsBodyAllocate,
	                   wren_PhysicsBodyFinalize, "PhysicsBody class wraps PhysicsBody component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "fixedRotation=(_)",
	                  wren_PhysicsBodySetFixedRotation, "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "linearVelocity",
	                  wren_PhysicsBodySetLinearVelocity, "Sets the linear velocity of a physics body.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "PhysicsBody", "setCollisionFilter(_,_)",
	                  wren_PhysicsBodySetCollisionFilter,
	                  "Sets which collision layer this body belongs to (categoryBits) and which layers it "
	                  "collides with (maskBits) - see physics.CollisionLayers for allocating layer bits by name.");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "PhysicsBody", "create(_,_,_)", wren_PhysicsBodyCreate,
	                  "Create a physics body");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "PhysicsBody", "get(_)", wren_PhysicsBodyGet,
	                  "Gets a physics body");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "PhysicsBody", "setLinearVelocity(_,_)",
	                  wren_PhysicsBodyStaticSetLinearVelocity, "Sets the linear velocity of a physics body.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "Shader", wren_ShaderAllocate, wren_ShaderFinalize,
	                   "Shader class wraps Shader component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Shader", "setFloatUniform(_,_)", wren_ShaderSetFloatUniform,
	                  "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Shader", "setIntUniform(_,_)", wren_ShaderSetIntUniform,
	                  "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Shader", "setVec2Uniform(_,_)", wren_ShaderSetVec2Uniform,
	                  "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Shader", "setVec3Uniform(_,_)", wren_ShaderSetVec3Uniform,
	                  "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Shader", "setVec4Uniform(_,_)", wren_ShaderSetVec4Uniform,
	                  "Sets the physics body fixed rotation");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Shader", "setMat4Uniform(_,_)", wren_ShaderSetMat4Uniform,
	                  "Sets the physics body fixed rotation");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Shader", "create(_,_)", wren_ShaderCreate,
	                  "Create a Shader body");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Shader", "get(_)", wren_ShaderGet, "Gets a Shader body");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "SpriteAnimation", wren_SpriteAnimationAllocate,
	                   wren_SpriteAnimationFinalize, "Sprite animation class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "SpriteAnimation", "addAnimation(_,_)",
	                  wren_SpriteAnimationAddAnimation, "Adds an animation to the sprite animation component");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "SpriteAnimation", "create(_)", wren_SpriteAnimationCreate,
	                  "Create a Sprite animation");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "SpriteAnimation", "get(_)", wren_SpriteAnimationGet,
	                  "Gets a Sprite animation");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "SpriteAnimation", "setCurrentAnimation(_,_)",
	                  wren_SpriteAnimationStaticSetCurrentAnimation,
	                  "Will set and play a current sprite animation, is already playing the animation continue it.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "SpriteAnimation", "forcePlayAnimation(_,_)",
	                  wren_SpriteAnimationPlayAnimation,
	                  "Will play a sprite animation, and if playering animation will forcibly restart it.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "SpriteAnimation", "isAnimationPlaying(_,_)",
	                  wren_SpriteAnimationIsAnimationPlaying, "Checks if a cirtain animation is playing.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "Sprite", wren_SpriteAllocate, wren_SpriteFinalize,
	                   "Sprite animation class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "texture", wren_SpriteGetTexture,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "texture=(_)", wren_SpriteSetTexture,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "color", wren_SpriteGetColor,
	                  "Sets the sprites color");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "color=(_)", wren_SpriteSetColor,
	                  "Sets the sprites color");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "offset", wren_SpriteGetOffset,
	                  "Sets the sprites offset");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "offset=(_)", wren_SpriteSetOffset,
	                  "Sets the sprites offset");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "columns", wren_SpriteGetColumns,
	                  "Sets the sprites columns");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "columns=(_)", wren_SpriteSetColumns,
	                  "Sets the sprites columns");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "rows", wren_SpriteGetRows, "Sets the sprites rows");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "rows=(_)", wren_SpriteSetRows,
	                  "Sets the sprites rows");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "flipped", wren_SpriteGetFlipped,
	                  "Sets the sprites flipped");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "flipped=(_)", wren_SpriteSetFlipped,
	                  "Sets the sprites flipped");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "index", wren_SpriteGetIndex,
	                  "Sets the sprites index");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "index=(_)", wren_SpriteSetIndex,
	                  "Sets the sprites index");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "layer", wren_SpriteGetLayer,
	                  "Gets the sprites render layer");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "layer=(_)", wren_SpriteSetLayer,
	                  "Sets the sprites render layer");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "orderInLayer", wren_SpriteGetOrderInLayer,
	                  "Gets the sprites sort order within its render layer");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Sprite", "orderInLayer=(_)", wren_SpriteSetOrderInLayer,
	                  "Sets the sprites sort order within its render layer");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Sprite", "create(_,_,_,_,_,_,_,_,_,_)", wren_SpriteCreate,
	                  "Creates the sprite Component.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Sprite", "get(_)", wren_SpriteGet, "Gets a Sprite Component");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Sprite", "setLayer(_,_)", wren_SpriteStaticSetLayer,
	                  "Sets the render layer of a sprite component");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Sprite", "setOrderInLayer(_,_)",
	                  wren_SpriteStaticSetOrderInLayer, "Sets the sort order within layer of a sprite component");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Sprite", "setFlipped(_,_)", wren_SpriteStaticSetFlipped,
	                  "Flips a sprite in a horizontal direction");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "LocalTransform", wren_LocalTransformAllocate,
	                   wren_LocalTransformFinalize, "Sprite animation class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "position", wren_LocalTransformGetPosition,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "position=(_)",
	                  wren_LocalTransformSetPosition, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "rotation", wren_LocalTransformGetRotation,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "rotation=(_)",
	                  wren_LocalTransformSetRotation, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "scale", wren_LocalTransformGetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "scale=(_)", wren_LocalTransformSetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "matrix", wren_LocalTransformGetMatrix,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "LocalTransform", "matrix=(_)", wren_LocalTransformSetMatrix,
	                  "Sets the sprites texture");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "get(_)", wren_LocalTransformGet,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getPosition(_)",
	                  wren_LocalTransformStaticGetPosition,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setPosition(_,_)",
	                  wren_LocalTransformStaticSetPosition, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getRotation(_)",
	                  wren_LocalTransformStaticGetRotation, "Get rotation of entity as Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setRotation(_,_)",
	                  wren_LocalTransformStaticSetRotation, "Set rotation of entity from Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getScale(_)",
	                  wren_LocalTransformStaticGetScale,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setScale(_,_)",
	                  wren_LocalTransformStaticSetScale, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "getMatrix(_)",
	                  wren_LocalTransformStaticGetMatrix,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "LocalTransform", "setMatrix(_,_)",
	                  wren_LocalTransformStaticSetMatrix, "Set the position of an entity.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "WorldTransform", wren_WorldTransformAllocate,
	                   wren_WorldTransformFinalize, "Sprite animation class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "position", wren_WorldTransformGetPosition,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "position=(_)",
	                  wren_WorldTransformSetPosition, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "rotation", wren_WorldTransformGetRotation,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "rotation=(_)",
	                  wren_WorldTransformSetRotation, "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "scale", wren_WorldTransformGetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "scale=(_)", wren_WorldTransformSetScale,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "matrix", wren_WorldTransformGetMatrix,
	                  "Sets the sprites texture");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "WorldTransform", "matrix=(_)", wren_WorldTransformSetMatrix,
	                  "Sets the sprites texture");

	// Register static methods
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "get(_)", wren_WorldTransformGet,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getPosition(_)",
	                  wren_WorldTransformStaticGetPosition,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setPosition(_,_)",
	                  wren_WorldTransformStaticSetPosition, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getRotation(_)",
	                  wren_WorldTransformStaticGetRotation, "Get rotation of entity as Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setRotation(_,_)",
	                  wren_WorldTransformStaticSetRotation, "Set rotation of entity from Quat.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getScale(_)",
	                  wren_WorldTransformStaticGetScale,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setScale(_,_)",
	                  wren_WorldTransformStaticSetScale, "Set the position of an entity.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "getMatrix(_)",
	                  wren_WorldTransformStaticGetMatrix,
	                  "Get the position of an entity. Returns vec3 or null if no transform.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "WorldTransform", "setMatrix(_,_)",
	                  wren_WorldTransformStaticSetMatrix, "Set the position of an entity.");

	// Register BodyDefinition foreign class
	WREN_FOREIGN_CLASS(registry, "gameObjectComponents", "Script", wren_WrenScriptAllocate, wren_WrenScriptFinalize,
	                   "Script class wraps SpriteAnimation component");

	// Register methods
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "getInstance()", wren_ScriptGetInstance,
	                  "Gets a script instance");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "isInitialised", wren_ScriptIsInitialised,
	                  "Checks if script is initialised");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "hasError", wren_ScriptHasError,
	                  "Checks if script has error");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Script", "errorMessage", wren_ScriptGetErrorMessage,
	                  "Gets Scripts error message");

	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Script", "create(_,_)", wren_ScriptCreate,
	                  "Creates the script Component.");
	WREN_CLASS_STATIC(
	    registry, "gameObjectComponents", "Script", "createArgPairs(_,_,_)", wren_ScriptCreateArgPairs,
	    "Internal: creates the script Component from a MapUtil-encoded arg map. Prefer createArg(_,_,_).");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Script", "get(_)", wren_WrenScriptGet,
	                  "Gets the script component");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "Script", "getInstance(_)", wren_ScriptStaticGetInstance,
	                  "Gets a script instance");

	WREN_IMPORT(registry, "gameObjectComponents", "serialisation", "MapUtil");
	WREN_IMPL(registry, "gameObjectComponents", "Script", true,
	          "static createArg(entity, className, args) {\n"
	          "    return createArgPairs(entity, className, MapUtil.mapToPairs(args))\n"
	          "}");
}
