// NOT COMPILED on this branch: the Wren bindings of the 2D world's components (camera, LDtk level and world,
// shader, sprite, sprite animation and particle emitter), moved here from WrenGameObjectComponents.cpp when the 2D
// pipeline left the build (docs/RideRefactorPlan.md). Restoring them means adding their entries back to
// ComponentListXMacro.h and these registrations to the module.

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

	wrenEnsureSlots(vm, 2);
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

void wren_CameraGetConfineToBounds(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, camera->component->confineToBounds);
}

void wren_CameraSetConfineToBounds(WrenVM* vm)
{
	WrenCamera* camera                 = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->component->confineToBounds = wrenGetSlotBool(vm, 1);
}

void wren_CameraGetBounds(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);

	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* vec4 = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (vec4) WrenVec4(camera->component->bounds);
}

void wren_CameraSetBounds(WrenVM* vm)
{
	WrenCamera* camera        = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	WrenVec4* bounds          = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	camera->component->bounds = bounds->value;
}

void wren_CameraGetViewCenter(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);

	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(camera->component->viewCenter);
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

void wren_LevelGetIdentifier(WrenVM* vm)
{
	WrenLevel* level = (WrenLevel*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotString(vm, 0, level->component->identifier.c_str());
}

void wren_LevelGetTags(WrenVM* vm)
{
	WrenLevel* level = (WrenLevel*)wrenGetSlotForeign(vm, 0);

	wrenSetSlotNewList(vm, 0);
	wrenEnsureSlots(vm, 3);
	const auto& tags = level->component->tags;
	for (int i = 0; i < static_cast<int>(tags.size()); i++)
	{
		wrenSetSlotString(vm, 2, tags[i].c_str());
		wrenInsertInList(vm, 0, i, 2);
	}
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

	int index = Struktur::FileLoading::LevelParser::FindLevelIndexByIdentifier(worldMap, levelName);

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

	int index = Struktur::FileLoading::LevelParser::FindLevelIndexByIdentifier(worldMap, levelName);

	if (index == -1)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	double doubleNumber = static_cast<double>(index);

	wrenSetSlotDouble(vm, 0, doubleNumber);
}

// Builds a Wren list of [index, dir] pairs (2-element lists, not maps - see note in the plan about
// avoiding map literals) in slot 0 for levelIndex's neighbours, resolving each neighbour's Iid to an
// index via FindLevelIndexByIid and silently skipping any that don't resolve.
void PushLevelNeighboursList(WrenVM* vm, const Struktur::FileLoading::LevelParser::World& worldMap, int levelIndex)
{
	wrenSetSlotNewList(vm, 0);
	wrenEnsureSlots(vm, 4);

	const auto& neighbours = worldMap.levels[levelIndex].neighbours;
	int outIndex           = 0;
	for (const auto& neighbour : neighbours)
	{
		int neighbourIndex = Struktur::FileLoading::LevelParser::FindLevelIndexByIid(worldMap, neighbour.levelIid);
		if (neighbourIndex == -1)
		{
			continue;
		}

		wrenSetSlotNewList(vm, 2);
		wrenSetSlotDouble(vm, 3, static_cast<double>(neighbourIndex));
		wrenInsertInList(vm, 2, 0, 3);
		wrenSetSlotString(vm, 3, neighbour.dir.c_str());
		wrenInsertInList(vm, 2, 1, 3);

		wrenInsertInList(vm, 0, outIndex, 2);
		outIndex++;
	}
}

// Puts levelIndex's custom field `name` in slot 0: a bool, number or string (enums as their value id), or null when
// the level has no such field, it was left empty, or its type isn't one scripts read (colour, point, tile).
void PushLevelField(WrenVM* vm, const Struktur::FileLoading::LevelParser::World& worldMap, int levelIndex,
                    const std::string& name)
{
	using Struktur::FileLoading::LevelParser::FieldInstanceType;
	wrenSetSlotNull(vm, 0);
	if (levelIndex < 0 || levelIndex >= static_cast<int>(worldMap.levels.size()))
	{
		return;
	}
	for (const auto& field : worldMap.levels[levelIndex].fieldInstances)
	{
		if (field.identifier != name || !field.value.has_value())
		{
			continue;
		}
		switch (field.type)
		{
			case FieldInstanceType::BOOLEAN:
				wrenSetSlotBool(vm, 0, std::any_cast<bool>(field.value));
				break;
			case FieldInstanceType::INTEGER:
				wrenSetSlotDouble(vm, 0, static_cast<double>(std::any_cast<int>(field.value)));
				break;
			case FieldInstanceType::FLOAT:
				wrenSetSlotDouble(vm, 0, static_cast<double>(std::any_cast<float>(field.value)));
				break;
			default:
				if (field.value.type() == typeid(std::string))
				{
					wrenSetSlotString(vm, 0, std::any_cast<std::string>(field.value).c_str());
				}
				break;
		}
		return;
	}
}

// Builds a Wren list of strings in slot 0 for levelIndex's tags.
void PushLevelTagsList(WrenVM* vm, const Struktur::FileLoading::LevelParser::World& worldMap, int levelIndex)
{
	wrenSetSlotNewList(vm, 0);
	wrenEnsureSlots(vm, 3);

	const auto& tags = worldMap.levels[levelIndex].tags;
	for (int i = 0; i < static_cast<int>(tags.size()); i++)
	{
		wrenSetSlotString(vm, 2, tags[i].c_str());
		wrenInsertInList(vm, 0, i, 2);
	}
}

// Builds a Wren list [worldX, worldY, pxWid, pxHei] in slot 0 for levelIndex - read straight from the
// parsed LDtk metadata, so it works for a level that isn't loaded yet (used to figure out which level
// a position falls in before deciding what to stream in).
void PushLevelBoundsList(WrenVM* vm, const Struktur::FileLoading::LevelParser::World& worldMap, int levelIndex)
{
	wrenSetSlotNewList(vm, 0);
	wrenEnsureSlots(vm, 3);

	const auto& level          = worldMap.levels[levelIndex];
	double values[4]           = {static_cast<double>(level.worldX), static_cast<double>(level.worldY),
                                  static_cast<double>(level.pxWid), static_cast<double>(level.pxHei)};
	for (int i = 0; i < 4; i++)
	{
		wrenSetSlotDouble(vm, 2, values[i]);
		wrenInsertInList(vm, 0, i, 2);
	}
}

// Reads a Wren list of strings out of the given slot.
std::vector<std::string> ReadStringList(WrenVM* vm, int listSlot, int scratchSlot)
{
	int count = wrenGetListCount(vm, listSlot);
	std::vector<std::string> result;
	result.reserve(count);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, listSlot, i, scratchSlot);
		result.push_back(wrenGetSlotString(vm, scratchSlot));
	}
	return result;
}

// First level whose tags contain every tag in requiredTags, or -1 if none match.
int FindLevelIndexWithAllTags(const Struktur::FileLoading::LevelParser::World& worldMap,
                              const std::vector<std::string>& requiredTags)
{
	for (int i = 0; i < static_cast<int>(worldMap.levels.size()); i++)
	{
		const auto& levelTags = worldMap.levels[i].tags;
		bool hasAll           = true;
		for (const auto& required : requiredTags)
		{
			if (std::find(levelTags.begin(), levelTags.end(), required) == levelTags.end())
			{
				hasAll = false;
				break;
			}
		}
		if (hasAll)
		{
			return i;
		}
	}
	return -1;
}

// World.getLevelNeighbours(levelIndex) -> list of [index, dir]
void wren_WorldGetLevelNeighbours(WrenVM* vm)
{
	WrenWorld* world    = (WrenWorld*)wrenGetSlotForeign(vm, 0);
	double levelDouble  = wrenGetSlotDouble(vm, 1);
	int levelIndex      = static_cast<int>(levelDouble);

	PushLevelNeighboursList(vm, world->component->worldMap, levelIndex);
}

// World.getLevelNeighbours(entity, levelIndex) -> list of [index, dir]
void wren_WorldStaticGetLevelNeighbours(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	double levelDouble  = wrenGetSlotDouble(vm, 2);
	int levelIndex      = static_cast<int>(levelDouble);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent)
	{
		wrenSetSlotNewList(vm, 0);
		return;
	}

	PushLevelNeighboursList(vm, worldComponent->worldMap, levelIndex);
}

// World.getLevelTags(levelIndex) -> list of string
void wren_WorldGetLevelTags(WrenVM* vm)
{
	WrenWorld* world   = (WrenWorld*)wrenGetSlotForeign(vm, 0);
	double levelDouble = wrenGetSlotDouble(vm, 1);
	int levelIndex     = static_cast<int>(levelDouble);

	PushLevelTagsList(vm, world->component->worldMap, levelIndex);
}

// World.getLevelTags(entity, levelIndex) -> list of string
void wren_WorldStaticGetLevelTags(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	double levelDouble  = wrenGetSlotDouble(vm, 2);
	int levelIndex      = static_cast<int>(levelDouble);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent)
	{
		wrenSetSlotNewList(vm, 0);
		return;
	}

	PushLevelTagsList(vm, worldComponent->worldMap, levelIndex);
}

// World.getLevelField(levelIndex, name) -> bool/num/string, or null
void wren_WorldGetLevelField(WrenVM* vm)
{
	WrenWorld* world = (WrenWorld*)wrenGetSlotForeign(vm, 0);
	int levelIndex   = static_cast<int>(wrenGetSlotDouble(vm, 1));
	std::string name = wrenGetSlotString(vm, 2);

	PushLevelField(vm, world->component->worldMap, levelIndex, name);
}

// World.getLevelField(entity, levelIndex, name) -> bool/num/string, or null
void wren_WorldStaticGetLevelField(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	entt::entity entity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	int levelIndex      = static_cast<int>(wrenGetSlotDouble(vm, 2));
	std::string name    = wrenGetSlotString(vm, 3);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	PushLevelField(vm, worldComponent->worldMap, levelIndex, name);
}

// World.getLevelBounds(levelIndex) -> list [worldX, worldY, pxWid, pxHei]
void wren_WorldGetLevelBounds(WrenVM* vm)
{
	WrenWorld* world   = (WrenWorld*)wrenGetSlotForeign(vm, 0);
	double levelDouble = wrenGetSlotDouble(vm, 1);
	int levelIndex     = static_cast<int>(levelDouble);

	PushLevelBoundsList(vm, world->component->worldMap, levelIndex);
}

// World.getLevelBounds(entity, levelIndex) -> list [worldX, worldY, pxWid, pxHei]
void wren_WorldStaticGetLevelBounds(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	double levelDouble  = wrenGetSlotDouble(vm, 2);
	int levelIndex      = static_cast<int>(levelDouble);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent)
	{
		wrenSetSlotNewList(vm, 0);
		return;
	}

	PushLevelBoundsList(vm, worldComponent->worldMap, levelIndex);
}

// World.getLevelIdentifier(entity, levelIndex) -> the level's LDtk identifier, or null
void wren_WorldStaticGetLevelIdentifier(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	entt::entity entity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	int levelIndex      = static_cast<int>(wrenGetSlotDouble(vm, 2));

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent || levelIndex < 0 || levelIndex >= static_cast<int>(worldComponent->worldMap.levels.size()))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotString(vm, 0, worldComponent->worldMap.levels[levelIndex].identifier.c_str());
}

// World.getLevelFieldColor(entity, levelIndex, name) -> [r, g, b] (0-255): the LDtk colour of the enum value a level's
// enum field holds (a level's Terrain -> the BattleArena enum's colour for it), or null.
void wren_WorldStaticGetLevelFieldColor(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	entt::entity entity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	int levelIndex      = static_cast<int>(wrenGetSlotDouble(vm, 2));
	std::string name    = wrenGetSlotString(vm, 3);

	wrenSetSlotNull(vm, 0);
	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent || levelIndex < 0 || levelIndex >= static_cast<int>(worldComponent->worldMap.levels.size()))
	{
		return;
	}
	const auto& worldMap = worldComponent->worldMap;
	for (const auto& field : worldMap.levels[levelIndex].fieldInstances)
	{
		if (field.identifier != name || field.type != Struktur::FileLoading::LevelParser::FieldInstanceType::ENUM ||
		    !field.value.has_value() || field.value.type() != typeid(std::string))
		{
			continue;
		}
		auto enumIt = worldMap.enumColors.find(field.enumType);
		if (enumIt == worldMap.enumColors.end())
		{
			return;
		}
		auto valueIt = enumIt->second.find(std::any_cast<std::string>(field.value));
		if (valueIt == enumIt->second.end())
		{
			return;
		}
		wrenSetSlotNewList(vm, 0);
		wrenEnsureSlots(vm, 2);
		for (int i = 0; i < 3; i++)
		{
			wrenSetSlotDouble(vm, 1, static_cast<double>(valueIt->second[i]));
			wrenInsertInList(vm, 0, i, 1);
		}
		return;
	}
}

// World.findLevelIndexWithTags(tagsList) -> number
void wren_WorldFindLevelIndexWithTags(WrenVM* vm)
{
	WrenWorld* world = (WrenWorld*)wrenGetSlotForeign(vm, 0);

	wrenEnsureSlots(vm, 3);
	std::vector<std::string> tags = ReadStringList(vm, 1, 2);

	int index = FindLevelIndexWithAllTags(world->component->worldMap, tags);
	if (index == -1)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotDouble(vm, 0, static_cast<double>(index));
}

// World.findLevelIndexWithTags(entity, tagsList) -> number
void wren_WorldStaticFindLevelIndexWithTags(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 4);
	std::vector<std::string> tags = ReadStringList(vm, 2, 3);

	int index = FindLevelIndexWithAllTags(worldComponent->worldMap, tags);
	if (index == -1)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenSetSlotDouble(vm, 0, static_cast<double>(index));
}

// World.getLoadedLevelEntity(levelIndex) -> number
void wren_WorldGetLoadedLevelEntity(WrenVM* vm)
{
	WrenWorld* world   = (WrenWorld*)wrenGetSlotForeign(vm, 0);
	double levelDouble = wrenGetSlotDouble(vm, 1);
	int levelIndex     = static_cast<int>(levelDouble);

	auto it = world->component->loadedLevels.find(levelIndex);
	if (it == world->component->loadedLevels.end())
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotDouble(vm, 0, static_cast<double>(it->second));
}

// World.getLoadedLevelEntity(entity, levelIndex) -> number
void wren_WorldStaticGetLoadedLevelEntity(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry       = context->GetRegistry();

	double entityId     = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	double levelDouble   = wrenGetSlotDouble(vm, 2);
	int levelIndex       = static_cast<int>(levelDouble);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);
	if (!worldComponent)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	auto it = worldComponent->loadedLevels.find(levelIndex);
	if (it == worldComponent->loadedLevels.end())
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotDouble(vm, 0, static_cast<double>(it->second));
}

// World.unloadLevelEntities(levelIndex)
void wren_WorldUnloadLevelEntities(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenWorld* world                = (WrenWorld*)wrenGetSlotForeign(vm, 0);

	double levelDouble = wrenGetSlotDouble(vm, 1);
	int levelIndex     = static_cast<int>(levelDouble);

	Struktur::World::Level::UnloadLevelEntities(*context, world->entity, levelIndex);
}

// World.unloadLevelEntities(entity, levelIndex)
void wren_WorldStaticUnloadLevelEntities(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	double entityId          = wrenGetSlotDouble(vm, 1);
	entt::entity worldEntity = static_cast<entt::entity>(entityId);
	double levelDouble        = wrenGetSlotDouble(vm, 2);
	int levelIndex            = static_cast<int>(levelDouble);

	Struktur::World::Level::UnloadLevelEntities(*context, worldEntity, levelIndex);
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

// SpriteAnimation.playAnimationAt(entity, animationName, fraction)
void wren_SpriteAnimationPlayAnimationAt(WrenVM* vm)
{
	Struktur::GameContext* context                 = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem                          = systemManager.GetSystem<Struktur::System::AnimationSystem>();

	double entityId           = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	double fraction           = wrenGetSlotDouble(vm, 3);
	entt::entity entity       = static_cast<entt::entity>(entityId);

	animationSystem.PlayAnimationAt(*context, entity, animationName, static_cast<float>(fraction));
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

// ParticleEmitter.font = Font - the font text particles draw in (see spawnText).
void wren_ParticleEmitterSetFont(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenFontHandle* font         = (WrenFontHandle*)wrenGetSlotForeign(vm, 1);
	emitter->component->font     = font->resource;
}

void wren_ParticleEmitterSetFontSize(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->fontSize = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

void wren_ParticleEmitterGetFontSize(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, static_cast<double>(emitter->component->fontSize));
}

void wren_ParticleEmitterSetOutlineColor(WrenVM* vm)
{
	WrenParticleEmitter* emitter      = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	WrenVec4* color                   = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	emitter->component->outlineColor = Struktur::Util::Color{(unsigned char)color->value.r, (unsigned char)color->value.g,
	                                                         (unsigned char)color->value.b, (unsigned char)color->value.a};
}

// ParticleEmitter.spawnText(text) - one text particle carrying `text`, spawned next update without clearing the
// particles already alive.
void wren_ParticleEmitterSpawnText(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->pendingTexts.emplace_back(wrenGetSlotString(vm, 1));
}

// ParticleEmitter.spawn(count) - `count` more particles next update, without clearing the ones alive.
void wren_ParticleEmitterSpawn(WrenVM* vm)
{
	WrenParticleEmitter* emitter      = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->pendingSpawns += std::max(0, static_cast<int>(wrenGetSlotDouble(vm, 1)));
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

// ParticleEmitter.setAnimation(definition, mode) - definition: a SpriteAnimationDefinition; mode: a
// ParticleAnimationMode value.
void wren_ParticleEmitterSetAnimationWithMode(WrenVM* vm)
{
	WrenParticleEmitter* emitter   = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	auto* definition               = static_cast<WrenSpriteAnimationDefinition*>(wrenGetSlotForeign(vm, 1));
	emitter->component->animation  = definition->spriteAnimation;
	int mode                       = static_cast<int>(wrenGetSlotDouble(vm, 2));
	emitter->component->animationMode =
	    static_cast<Struktur::Component::ParticleAnimationMode>(std::clamp(mode, 0, 3));
}

// ParticleEmitter.setAnimation(definition) - plays it like a sprite would: Loop if the definition loops, else Once.
void wren_ParticleEmitterSetAnimation(WrenVM* vm)
{
	WrenParticleEmitter* emitter  = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	auto* definition              = static_cast<WrenSpriteAnimationDefinition*>(wrenGetSlotForeign(vm, 1));
	emitter->component->animation = definition->spriteAnimation;
	emitter->component->animationMode = definition->spriteAnimation.loop
	                                        ? Struktur::Component::ParticleAnimationMode::Loop
	                                        : Struktur::Component::ParticleAnimationMode::Once;
}

// ParticleEmitter.clearAnimation() - back to no animation on cell 0.
void wren_ParticleEmitterClearAnimation(WrenVM* vm)
{
	WrenParticleEmitter* emitter      = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	emitter->component->animation     = Struktur::Animation::SpriteAnimation{0, 1, 1.0f, false};
	emitter->component->animationMode = Struktur::Component::ParticleAnimationMode::None;
}

void wren_ParticleEmitterSetAnimationMode(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	int mode                     = static_cast<int>(wrenGetSlotDouble(vm, 1));
	emitter->component->animationMode =
	    static_cast<Struktur::Component::ParticleAnimationMode>(std::clamp(mode, 0, 3));
}

void wren_ParticleEmitterGetAnimationMode(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, static_cast<double>(emitter->component->animationMode));
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

// ParticleEmitter.emit() - (re-)arms the one-shot burst so the next System::ParticleSystem::Update
// spawns burstCount particles fresh, clearing any particles still alive from a previous burst first
// so a rapid re-trigger doesn't blend two bursts together. Lets one persistent emitter (e.g. one per
// battler in combat) be reused for repeated effects instead of creating/destroying an emitter per hit.
void wren_ParticleEmitterEmit(WrenVM* vm)
{
	WrenParticleEmitter* emitter = (WrenParticleEmitter*)wrenGetSlotForeign(vm, 0);
	for (auto& particle : emitter->component->particles)
	{
		particle.alive = false;
	}
	emitter->component->spawnAccumulator = 0.0f;
	emitter->component->hasBurst          = false;
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
	auto flipped               = static_cast<Struktur::Renderer::FlipBit>(static_cast<int>(wrenGetSlotDouble(vm, 7)));
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
	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "resourceManager", "Texture", 1);  // Get class into slot 1
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
	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
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
	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
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
	wrenSetSlotDouble(vm, 0, static_cast<double>(sprite->component->flipped));
}

void wren_SpriteSetFlipped(WrenVM* vm)
{
	WrenSprite* sprite = (WrenSprite*)wrenGetSlotForeign(vm, 0);
	auto flipped        = static_cast<Struktur::Renderer::FlipBit>(static_cast<int>(wrenGetSlotDouble(vm, 1)));
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
	auto flipped        = static_cast<Struktur::Renderer::FlipBit>(static_cast<int>(wrenGetSlotDouble(vm, 2)));
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->flipped = flipped;
}


// --- registrations -------------------------------------
#if 0

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
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "font=(_)", wren_ParticleEmitterSetFont,
	                  "Set the font text particles draw in (see spawnText)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "fontSize", wren_ParticleEmitterGetFontSize,
	                  "Get text particles' size in world pixels");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "fontSize=(_)", wren_ParticleEmitterSetFontSize,
	                  "Set text particles' size in world pixels (a pixel font snaps to whole multiples of its own size)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "outlineColor=(_)",
	                  wren_ParticleEmitterSetOutlineColor, "Set text particles' one-pixel outline colour (alpha 0 = none)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "spawnText(_)", wren_ParticleEmitterSpawnText,
	                  "Spawn one text particle showing the given text, keeping the particles already alive");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "spawn(_)", wren_ParticleEmitterSpawn,
	                  "Spawn this many more particles, keeping the particles already alive");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "setAnimation(_,_)",
	                  wren_ParticleEmitterSetAnimationWithMode,
	                  "Set the sprite animation (a SpriteAnimationDefinition) each particle plays, and how (a ParticleAnimationMode)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "setAnimation(_)",
	                  wren_ParticleEmitterSetAnimation,
	                  "Set the sprite animation each particle plays - looped if the definition loops, otherwise once");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "clearAnimation()",
	                  wren_ParticleEmitterClearAnimation, "Stop animating particles - they draw cell 0");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "animationMode",
	                  wren_ParticleEmitterGetAnimationMode, "Get how particles play their animation (a ParticleAnimationMode)");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "animationMode=(_)",
	                  wren_ParticleEmitterSetAnimationMode, "Set how particles play their animation (a ParticleAnimationMode)");
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
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "ParticleEmitter", "emit()", wren_ParticleEmitterEmit,
	                  "(Re-)arm the one-shot burst, clearing any still-alive particles first");

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
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "confineToBounds", wren_CameraGetConfineToBounds,
	                  "Get whether the view is kept inside bounds");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "confineToBounds=(_)",
	                  wren_CameraSetConfineToBounds, "Set whether the view is kept inside bounds");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "bounds", wren_CameraGetBounds,
	                  "Get the bounds as a Vec4 (world x, y, width, height) the view is kept inside when confineToBounds is set");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "bounds=(_)", wren_CameraSetBounds,
	                  "Set the bounds as a Vec4 (world x, y, width, height) the view is kept inside when confineToBounds is set");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Camera", "viewCenter", wren_CameraGetViewCenter,
	                  "Get the world point at the centre of the view as of the last camera update (before shake)");
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
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Level", "identifier", wren_LevelGetIdentifier,
	                  "Get the level's identifier (name).");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "Level", "tags", wren_LevelGetTags,
	                  "Get the level's tags as a list of strings.");

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
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLevelNeighbours(_)",
	                  wren_WorldGetLevelNeighbours,
	                  "Get a level's neighbours as a list of [index, dir], resolved from the LDtk world file.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLevelTags(_)", wren_WorldGetLevelTags,
	                  "Get a level's tags as a list of strings.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLevelField(_,_)", wren_WorldGetLevelField,
	                  "Get one of a level's custom LDtk fields by name (bool/num/string), or null if it has none.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLevelBounds(_)", wren_WorldGetLevelBounds,
	                  "Get a level's [worldX, worldY, pxWid, pxHei] from the parsed LDtk metadata, even if unloaded.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "findLevelIndexWithTags(_)",
	                  wren_WorldFindLevelIndexWithTags,
	                  "Get the index of the first level whose tags contain every tag in the given list.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "getLoadedLevelEntity(_)",
	                  wren_WorldGetLoadedLevelEntity, "Get the entity for a level index if it's currently loaded.");
	WREN_CLASS_METHOD(registry, "gameObjectComponents", "World", "unloadLevelEntities(_)",
	                  wren_WorldUnloadLevelEntities, "Destroys a loaded level's entities and frees it.");

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
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelNeighbours(_,_)",
	                  wren_WorldStaticGetLevelNeighbours,
	                  "Get a level's neighbours as a list of [index, dir], resolved from the LDtk world file.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelTags(_,_)", wren_WorldStaticGetLevelTags,
	                  "Get a level's tags as a list of strings.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelField(_,_,_)", wren_WorldStaticGetLevelField,
	                  "Get one of a level's custom LDtk fields by name (bool/num/string), or null if it has none.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelBounds(_,_)", wren_WorldStaticGetLevelBounds,
	                  "Get a level's [worldX, worldY, pxWid, pxHei] from the parsed LDtk metadata, even if unloaded.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelIdentifier(_,_)",
	                  wren_WorldStaticGetLevelIdentifier, "Get a level's LDtk identifier, or null.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLevelFieldColor(_,_,_)",
	                  wren_WorldStaticGetLevelFieldColor,
	                  "Get the LDtk colour [r, g, b] of the enum value in one of a level's enum fields, or null.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "findLevelIndexWithTags(_,_)",
	                  wren_WorldStaticFindLevelIndexWithTags,
	                  "Get the index of the first level whose tags contain every tag in the given list.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "getLoadedLevelEntity(_,_)",
	                  wren_WorldStaticGetLoadedLevelEntity,
	                  "Get the entity for a level index if it's currently loaded.");
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "World", "unloadLevelEntities(_,_)",
	                  wren_WorldStaticUnloadLevelEntities, "Destroys a loaded level's entities and frees it.");

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
	WREN_CLASS_STATIC(registry, "gameObjectComponents", "SpriteAnimation", "playAnimationAt(_,_,_)",
	                  wren_SpriteAnimationPlayAnimationAt,
	                  "Will play a sprite animation as if it had already played for the given fraction (0..1) of "
	                  "its duration, restarting it if it is already playing.");
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

	WREN_ENUM(registry, "gameObjectComponents", RenderLayer, "Coarse draw-order buckets for sprites and tile layers",
	          WREN_ENUM_PAIR("BACKGROUND_FAR", Struktur::World::RenderLayer::BackgroundFar),
	          WREN_ENUM_PAIR("BACKGROUND_MID", Struktur::World::RenderLayer::BackgroundMid),
	          WREN_ENUM_PAIR("ENTITIES", Struktur::World::RenderLayer::Entities),
	          WREN_ENUM_PAIR("BACKGROUND_OVERLAY", Struktur::World::RenderLayer::BackgroundOverlay),
	          WREN_ENUM_PAIR("FOREGROUND", Struktur::World::RenderLayer::Foreground),
	          WREN_ENUM_PAIR("UI", Struktur::World::RenderLayer::UI), );

	WREN_ENUM(registry, "gameObjectComponents", ParticleAnimationMode,
	          "How each particle plays its emitter's sprite animation (ParticleEmitter.setAnimation)",
	          WREN_ENUM_PAIR("NONE", Struktur::Component::ParticleAnimationMode::None),
	          WREN_ENUM_PAIR("ONCE", Struktur::Component::ParticleAnimationMode::Once),
	          WREN_ENUM_PAIR("LOOP", Struktur::Component::ParticleAnimationMode::Loop),
	          WREN_ENUM_PAIR("LIFETIME", Struktur::Component::ParticleAnimationMode::Lifetime), );

#endif
