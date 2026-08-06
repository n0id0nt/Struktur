#pragma once

#include <vector>

#include "Engine/Scripting/WrenValueWrapper.h"
#include "wren.hpp"

namespace Struktur::Wren::Util
{
// Decodes a Wren "pairs" list (as produced by MapUtil.mapToPairs - a [["__type","map"], [key,value], ...] encoded
// map) at `pairsSlot` into a flat list of items. Nested maps/lists are not supported; only primitive values.
std::vector<Wren::WrenItem> GetWrenMapFromPairs(WrenVM* vm, int pairsSlot, int firstClearSlot);
}
