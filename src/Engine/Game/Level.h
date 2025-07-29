#pragma once

#include <string>
#include "entt/entt.hpp"

namespace Struktur
{
	class GameContext;

	namespace GameResource
	{
		namespace Level
		{
			entt::entity CreateWorldEntity(GameContext& context, const std::string& filePath);
			entt::entity LoadLevelEntities(GameContext& context, const entt::entity worldEntity, int levelIndex);
		}
	}
}

