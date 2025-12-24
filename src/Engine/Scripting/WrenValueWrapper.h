#pragma once

#include "wren.hpp"

#include <string>
#include <any>

namespace Struktur::Wren
{

struct WrenItem
{
    std::string identifier;
    std::any value;
    WrenType type;
};

}