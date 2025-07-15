#pragma once

#include <entt/entt.hpp>

#include "Engine/FileLoading/LevelParser.h"

namespace Struktur
{
	class GameContext;

	namespace GameResource
	{
		namespace Level
		{
			void LoadLevelEntities(GameContext& context, const Struktur::FileLoading::LevelParser::Level& level);
		}
	}
}

