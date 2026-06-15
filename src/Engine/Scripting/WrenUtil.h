#pragma once

#include <vector>

#include "Engine/Scripting/WrenValueWrapper.h"
#include "wren.hpp"

namespace Struktur::Wren::Util
{
std::vector<Wren::WrenItem> GetWrenMapDoubleList(WrenVM* vm, int keysSlot, int valuesSlot, int firstClearSlot);
}
