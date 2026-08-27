#pragma once

#include <any>
#include <string>

#include "wren.hpp"

namespace Struktur::Wren
{
struct WrenItem
{
	std::string identifier;
	std::any value;
	WrenType type;
};
}  // namespace Struktur::Wren
