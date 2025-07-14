#pragma once

#include <entt/entt.hpp>

#include "FileLoading/LevelParser.h"

namespace Struktur
{
	class GameContext;

	namespace Game
	{
		namespace Level
		{
			void LoadLevelEntities(GameContext& context, const Struktur::FileLoading::LevelParser::Level& level);
		}
	};
};

