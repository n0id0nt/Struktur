#include "TileMapCollisionBodyGenerator.h"

#include <unordered_set>

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/PhysicsBody.h"

void Struktur::Physics::TileMapCollisionBodyGenerator::CreateTileMapShape(GameContext &context, const Component::TileMap &tilemap, bool isSensor, Component::PhysicsBody &out_body)
{
    Physics::PhysicsWorld& world = context.GetPhysicsWorld();

    b2Filter filter;
    //filter.categoryBits = categoryBits;
    //filter.maskBits = maskBits;

    float scale = world.GetPixelsPerMeter();

    std::unordered_set<glm::ivec2, IVec2Hash> checkedTiles;

	for (unsigned int row = 0; row < tilemap.height; row++)
	{
		for (unsigned int col = 0; col < tilemap.width; col++)
		{
			unsigned int tile = GetTileAt(tilemap, row, col);
			glm::ivec2 tilePos{col, row};
			if (tile != 0 && !checkedTiles.contains(tilePos))
			{
				checkedTiles.insert(tilePos);
				if (IsTileClearAtDir(tilePos, tilemap, Dir::Left))
				{
					CreateLoop(out_body, Dir::Left, tilePos, checkedTiles, tilemap, isSensor, filter, scale);
				}
				else if (IsTileClearAtDir(tilePos, tilemap, Dir::Up))
				{
					CreateLoop(out_body, Dir::Up, tilePos, checkedTiles, tilemap, isSensor, filter, scale);
				}
				else if (IsTileClearAtDir(tilePos, tilemap, Dir::Right))
				{
					CreateLoop(out_body, Dir::Right, tilePos, checkedTiles, tilemap, isSensor, filter, scale);
				}
				else if (IsTileClearAtDir(tilePos, tilemap, Dir::Down))
				{
					CreateLoop(out_body, Dir::Down, tilePos, checkedTiles, tilemap, isSensor, filter, scale);
				}
			}
		}
	}	
}

void Struktur::Physics::TileMapCollisionBodyGenerator::CreateLoop(Component::PhysicsBody& out_body, Dir inputDir, glm::ivec2 inputTile, std::unordered_set<glm::ivec2, IVec2Hash> &checkedTiles, const Component::TileMap &tilemap, bool isSensor, const b2Filter &filter, float scale)
{
    std::vector<b2Vec2> vertices;

	b2FixtureDef fixtureDef;
	fixtureDef.friction = 0.f;
	fixtureDef.density = 1.f;
	fixtureDef.isSensor = isSensor;
	fixtureDef.filter = filter;

    b2ChainShape chainShape;

    std::array<b2Vec2, 2> line = GetSideLine(inputDir, inputTile, tilemap, scale);
    vertices.insert(vertices.end(), line.begin(), line.end());
    inputDir = NextDir(inputDir);

    while (!vertices.empty() && vertices.front() != vertices.back())
    {
        if (IsTileClearAtDir(inputTile, tilemap, inputDir))
        {
            line = GetSideLine(inputDir, inputTile, tilemap, scale);
            vertices.push_back(line.back());
            inputDir = NextDir(inputDir);
        }
        else
        {
            inputTile = GetTileInDir(inputTile, inputDir);
            checkedTiles.insert(inputTile);
            inputDir = PreviousDir(inputDir);
        }
    }

    vertices.pop_back();

	chainShape.CreateLoop(vertices.data(), vertices.size());
	fixtureDef.shape = &chainShape;

	out_body.body->CreateFixture(&fixtureDef);
}

int Struktur::Physics::TileMapCollisionBodyGenerator::GetTileAt(const Component::TileMap &tilemap, unsigned int row, unsigned int col)
{
    return tilemap.grid[row * tilemap.width + col];
}

bool Struktur::Physics::TileMapCollisionBodyGenerator::IsTileClearAtDir(glm::ivec2 tile, const Component::TileMap &tilemap, Dir inputDir)
{
    switch (inputDir)
	{
	case Dir::Left:
		return tile.x - 1 < 0 || GetTileAt(tilemap, tile.y, tile.x - 1) == 0;
	case Dir::Up:
		return tile.y - 1 < 0 || GetTileAt(tilemap, tile.y - 1, tile.x) == 0;
	case Dir::Right:
		return tile.x >= tilemap.width - 1 || GetTileAt(tilemap, tile.y, tile.x + 1) == 0;
	case Dir::Down:
		return tile.y >= tilemap.height - 1 || GetTileAt(tilemap, tile.y + 1, tile.x) == 0;
	}
	return false;
}

Struktur::Physics::TileMapCollisionBodyGenerator::Dir Struktur::Physics::TileMapCollisionBodyGenerator::NextDir(Dir inputDir)
{
    return static_cast<Dir>(((int)inputDir + 1) % (int)Dir::Count);
}

Struktur::Physics::TileMapCollisionBodyGenerator::Dir Struktur::Physics::TileMapCollisionBodyGenerator::PreviousDir(Dir inputDir)
{
    int newDir = (int)inputDir - 1;
    if (newDir < 0)
    {
        newDir = (int)Dir::Count - 1;
    }
    return static_cast<Dir>(newDir);
}

bool Struktur::Physics::TileMapCollisionBodyGenerator::IsNextDirClear(glm::ivec2 tile, const Component::TileMap &tilemap, Dir inputDir)
{
    return IsTileClearAtDir(tile, tilemap, NextDir(inputDir));
}

glm::ivec2 Struktur::Physics::TileMapCollisionBodyGenerator::GetTileInDir(glm::ivec2 tile, Dir inputDir)
{
    switch (inputDir)
    {
    case Dir::Left:
        tile.x -= 1;
        break;
    case Dir::Up:
        tile.y -= 1;
        break;
    case Dir::Right:
        tile.x += 1;
        break;
    case Dir::Down:
        tile.y += 1;
        break;
    }
    return tile;
}

std::array<b2Vec2, 2> Struktur::Physics::TileMapCollisionBodyGenerator::GetSideLine(Dir inputDir, glm::ivec2 inputTile, const Component::TileMap &tilemap, float scale)
{
    switch (inputDir)
	{
	case Dir::Right:
		return std::array<b2Vec2, 2>{
			b2Vec2{ (inputTile.x * tilemap.tileSize + tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize) / scale },
			b2Vec2{ (inputTile.x * tilemap.tileSize + tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize + tilemap.tileSize) / scale },
		};
	case Dir::Up:
		return std::array<b2Vec2, 2>{
			b2Vec2{ (inputTile.x * tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize) / scale },
			b2Vec2{ (inputTile.x * tilemap.tileSize + tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize) / scale },
		};
	case Dir::Left:
		return std::array<b2Vec2, 2>{
			b2Vec2{ (inputTile.x * tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize + tilemap.tileSize) / scale },
			b2Vec2{ (inputTile.x * tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize) / scale },
		};
	case Dir::Down:
		return std::array<b2Vec2, 2>{
			b2Vec2{ (inputTile.x * tilemap.tileSize + tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize + tilemap.tileSize) / scale },
			b2Vec2{ (inputTile.x * tilemap.tileSize) / scale, (inputTile.y * tilemap.tileSize + tilemap.tileSize) / scale },
		};
	}
	return std::array<b2Vec2, 2>{};
}
