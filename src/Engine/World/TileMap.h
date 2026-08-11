#pragma once

#include "glm/glm.hpp"

namespace Struktur
{
namespace World
{
namespace TileMap
{
enum class FlipBit
{
	NONE       = 0,
	HORIZONTAL = 1,
	VERTIAL    = 2,
	BOTH       = 3,
	COUNT
};

struct GridTile
{
	glm::vec2 position;
	glm::vec2 sourcePosition;
	FlipBit flipBit;
};
};  // namespace TileMap
}  // namespace World
}  // namespace Struktur
