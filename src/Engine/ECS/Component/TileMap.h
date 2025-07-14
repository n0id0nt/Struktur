#pragma once

#include <string>
#include <vector>

#include "Game/TileMap.h"

namespace Struktur
{
	namespace Component
	{
        struct TileMap
		{
			std::string imagePath;
			int width;
			int height;
			int tileSize;
			std::vector<Game::TileMap::GridTile> gridTiles;
			std::vector<int> grid;
		};
    }
}