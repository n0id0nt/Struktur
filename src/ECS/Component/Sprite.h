#pragma once

#include <string>
#include "raylib.h"
#include "glm/glm.hpp"

namespace Struktur
{
	namespace Component
	{
        struct Sprite {
            std::string fileName;
            Color color;
            glm::vec2 offset;
            int columns, rows;
			bool flipped; // TODO change this to an enum
			int index;
        };
    }
}