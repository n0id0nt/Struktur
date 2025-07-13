#pragma once

#include <entt/entt.hpp>

#include "FileLoading/LevelParser.h"

namespace Struktur
{
    namespace Core
    {
        class GameContext;
    }
	namespace Game
	{
		namespace Level
		{
			void LoadLevelEntities(Core::GameContext& context, const Struktur::FileLoading::LevelParser::Level& level);
		}
	};
};

