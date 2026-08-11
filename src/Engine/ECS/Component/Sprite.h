#pragma once

#include <string>

#include "Engine/World/RenderLayer.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/Util/Color.h"
#include "glm/glm.hpp"

namespace Struktur
{
namespace Component
{
struct Sprite
{
	Resource::ResourcePtr<Resource::TextureResource> texture;
	Util::Color color;
	glm::vec2 offset;

	// TODO also store a cached version of the source rec here so it doesn't need to be recalculated
	int columns, rows;
	bool flipped;  // TODO change this to an enum
	int index;
	Struktur::World::RenderLayer layer;
	// Manual sort bias within layer. For RenderLayer::Entities the render system adds world Y to this,
	// so dynamic entities y-sort against whichever tile layers sit immediately behind/in front of them.
	float orderInLayer;
};
}  // namespace Component
}  // namespace Struktur
