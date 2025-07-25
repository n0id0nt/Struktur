#pragma once

#include <string>
#include <vector>

#include "Engine/Game/TileMap.h"
#include "Engine/Core/Resource/TextureResource.h"

namespace Struktur
{
	namespace Component
	{
        struct TileMap
		{
			Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture;
			int width;
			int height;
			int tileSize;
			std::vector<GameResource::TileMap::GridTile> gridTiles;
			std::vector<int> grid;
		};
    }
}