#pragma once

#include "box2d/box2d.h"
#include "glm/glm.hpp"

#include "Engine/Physics/ContactListener.h"

namespace Struktur
{
    class GameContext;

    namespace Component
	{
        struct TileMap;
        struct PhysicsBody;
	}
	namespace Physics
	{
        namespace TileMapCollisionBodyGenerator
        {
            enum class Dir 
            {
                Right,
                Down,
                Left,
                Up,

                Count
            };

            // Custom hash function for glm::ivec2 --TODO Move to seperate Helper file
            struct IVec2Hash
            {
                std::size_t operator()(const glm::ivec2& v) const 
                {
                    // Combine hash values of each component
                    std::hash<unsigned int> hasher;
                    std::size_t hash = hasher(v.x);
                    hash ^= hasher(v.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    return hash;
                }
            };

            void CreateTileMapShape(GameContext &context, const Component::TileMap &tilemap, bool isSensor, Component::PhysicsBody &out_body);
            
            void CreateLoop(Component::PhysicsBody& out_body, Dir inputDir, glm::ivec2 inputTile, std::unordered_set<glm::ivec2, IVec2Hash>& checkedTiles, const Component::TileMap& tilemap, bool isSensor, const b2Filter& filter, float scale);
            // TODO move to seperate file too usefull to just be here
            int GetTileAt(const Component::TileMap& tilemap, unsigned int row, unsigned int col);
            bool IsTileClearAtDir(glm::ivec2 tile, const Component::TileMap& tilemap, Dir inputDir);
            Dir NextDir(Dir inputDir);
            Dir PreviousDir(Dir inputDir);
            bool IsNextDirClear(glm::ivec2 tile, const Component::TileMap& tilemap, Dir inputDir);
            glm::ivec2 GetTileInDir(glm::ivec2 tile, Dir inputDir);
            std::array<b2Vec2, 2> GetSideLine(Dir inputDir, glm::ivec2 inputTile, const Component::TileMap& tilemap, float scale);
        }
    }
}

