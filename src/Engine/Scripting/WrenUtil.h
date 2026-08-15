#pragma once

#include <vector>

#include "Engine/Scripting/WrenValueWrapper.h"
#include "wren.hpp"

namespace Struktur::Wren::Util
{
// Decodes a Wren "pairs" list (as produced by MapUtil.mapToPairs - a [["__type","map"], [key,value], ...] encoded
// map) at `pairsSlot` into a flat list of items. Nested maps/lists are not supported; only primitive values.
std::vector<Wren::WrenItem> GetWrenMapFromPairs(WrenVM* vm, int pairsSlot, int firstClearSlot);

// Single-value num/bool/string/null <-> WrenItem conversions, factored out of GetWrenMapFromPairs so
// callers that only need one slot (e.g. reading/writing an exported script field) don't need a list.
WrenItem GetWrenItemFromSlot(WrenVM* vm, int slot);
void SetSlotFromWrenItem(WrenVM* vm, int slot, const WrenItem& item);
}
