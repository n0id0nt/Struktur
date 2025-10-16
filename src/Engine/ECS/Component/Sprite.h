#pragma once

#include <string>
#include "raylib.h"
#include "glm/glm.hpp"
#include "Engine/Resource/TextureResource.h"

namespace Struktur
{
	namespace Component
	{
        struct Sprite
        {
            Resource::ResourcePtr<Resource::TextureResource> texture;
            ::Color color;
            glm::vec2 offset;

            // TODO also store a cached version of the source rec here so it doesn't need to be recalculated
            int columns, rows;
			bool flipped; // TODO change this to an enum
			int index;
            int renderPriority;
        };
    }
}
