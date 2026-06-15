#pragma once

#include <string>

#include "Engine/Callback/Variant.h"

namespace Struktur
{
namespace Event
{
struct Event
{
	std::string type;
	Struktur::Callback::Variant data;
};
}  // namespace Event
}  // namespace Struktur
