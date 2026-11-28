#pragma once

#include <string>
#include <vector>

#include "entt/entt.hpp"

#include "Engine/FileLoading/LevelParser.h"
#include "Engine/Scripting/WrenValueWrapper.h"

namespace Struktur
{
class GameContext;

namespace World
{
namespace Level
{
// What LoadLevelEntities does with each LDtk entity instance.
enum class EntityMode
{
	SCRIPT,   // Game: a GameObject with a WrenScript component, constructed and started like any script.
	PREVIEW,  // Editor level viewer: a GameObject with a LevelEntityInstance and whatever the script's
	          // `static preview(entity, args)` gives it (a sprite) - no WrenScript, so nothing ever runs on it.
	NONE,     // Skip entity instances entirely.

	COUNT
};

struct LevelLoadOptions
{
	// Builds the static body for the layers tagged "Collision" in LDtk (in games with physics).
	bool createPhysics    = true;
	EntityMode entityMode = EntityMode::SCRIPT;
};

entt::entity CreateWorldEntity(GameContext& context, const std::string& filePath);
entt::entity LoadLevelEntities(GameContext& context, const entt::entity worldEntity, int levelIndex,
                               const LevelLoadOptions& options = {});
void UnloadLevelEntities(GameContext& context, const entt::entity worldEntity, int levelIndex);
// The constructor args map a script receives for an LDtk entity instance: one item per field instance plus "Iid".
// An entity-ref field <Field> (the referenced iids, comma-separated) also gets "<Field>Names": the "Name" field of
// each referenced entity, looked up anywhere in world - e.g. a Key door's Keys -> the item ids it needs - and
// "<Field>Levels": the identifier of the level each one is in - e.g. a DoorWorp's Destination -> the room it leads to.
std::vector<Wren::WrenItem> BuildEntityScriptArgs(const FileLoading::LevelParser::Entity& entityInstance,
                                                  const FileLoading::LevelParser::World& world);
}  // namespace Level
}  // namespace World
}  // namespace Struktur
