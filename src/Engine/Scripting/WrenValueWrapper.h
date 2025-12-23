#include "wren.hpp"

#include <string>
#include <any>

namespace Struktur::Wren
{

struct Item
{
    std::string identifier;
    std::any value;
    WrenType type;
};

}