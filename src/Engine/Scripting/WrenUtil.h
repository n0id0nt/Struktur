#pragma once

#include <vector>
#include "wren.hpp"

#include "Engine/Scripting/WrenValueWrapper.h"

namespace Struktur::Wren::Util
{
    std::vector<Wren::WrenItem> GetWrenMapDoubleList(WrenVM* vm, int keysSlot, int valuesSlot, int firstClearSlot);
}