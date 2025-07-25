#pragma once

#include <string>
#include "raylib.h"
#include "glm/glm.hpp"
#include "Engine/Core/Resource/TextureResource.h"

namespace Struktur
{
	namespace Component
	{
        struct Sprite {
            Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture;
            ::Color color;
            glm::vec2 offset;

            // TODO also store a cached version of the source rec here so it doesn't need to be recalculated
            int columns, rows;
			bool flipped; // TODO change this to an enum
			int index;
        };
    }
}
