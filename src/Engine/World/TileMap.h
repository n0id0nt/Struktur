#pragma once

#include "Engine/Renderer/FlipBit.h"
#include "glm/glm.hpp"

namespace Struktur
{
namespace World
{
namespace TileMap
{
struct GridTile
{
	glm::vec2 position;
	glm::vec2 sourcePosition;
	Struktur::Renderer::FlipBit flipBit;
};
};  // namespace TileMap
}  // namespace World
}  // namespace Struktur
