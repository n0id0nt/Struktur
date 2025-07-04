#pragma once

#include <string>
#include "raylib.h"

namespace Struktur
{
	namespace Component
	{
        struct Sprite {
            std::string fileName;
            Color color;
        };
    }
}