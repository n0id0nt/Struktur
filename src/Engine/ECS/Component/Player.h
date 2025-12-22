#pragma once

#include "glm/glm.hpp"

namespace Struktur
{
	namespace Component
	{
        struct Player
        {
            float speed;
            glm::vec2 facing;
        };
    }
}
