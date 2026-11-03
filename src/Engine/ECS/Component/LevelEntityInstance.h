#pragma once

#include <string>
#include <vector>

#include "Engine/Scripting/WrenValueWrapper.h"
#include "Engine/Util/Color.h"
#include "glm/glm.hpp"

namespace Struktur
{
namespace Component
{
// An LDtk entity instance as the editor's level viewer loads it (World::Level::EntityMode::PREVIEW): what it is, for
// picking and the Inspector, and - if its script's preview gave it no Sprite - the rect DebugSystem draws instead.
struct LevelEntityInstance
{
	std::string identifier;
	std::string Iid;
	// The args its script would be constructed with (see World::Level::BuildEntityScriptArgs).
	std::vector<Wren::WrenItem> fields;
	// LDtk bounds: top-left corner relative to the entity's position, and size - both px.
	glm::vec2 offset;
	glm::vec2 size;
	Util::Color color;
};
}  // namespace Component
}  // namespace Struktur
