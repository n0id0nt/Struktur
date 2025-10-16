#pragma once

#include <string>
#include <vector>

#include "Engine/Game/TileMap.h"
#include "Engine/Resource/TextureResource.h"

namespace Struktur
{
	namespace Component
	{
        struct TileMap
		{
			Resource::ResourcePtr<Resource::TextureResource> texture;
			int width;
			int height;
			int tileSize;
			std::vector<GameResource::TileMap::GridTile> gridTiles;
			std::vector<int> grid;
		};
    }
}