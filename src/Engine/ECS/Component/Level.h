#pragma once

#include "Engine/FileLoading/LevelParser.h"

namespace Struktur
{
	namespace Component
	{
        struct Level
        {
            int index;
            std::string Iid;
            int width;
            int height; 
        };

        struct World
        {
            FileLoading::LevelParser::World worldMap;
        };
    }
}