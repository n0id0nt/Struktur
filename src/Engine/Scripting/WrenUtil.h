#pragma once

#include <vector>
#include <Wren.hpp>

#include "Engine/Scripting/WrenValueWrapper.h"

namespace Struktur::Wren::Util
{
    std::vector<Wren::WrenItem> GetWrenMapDoubleList(WrenVM* vm, int keysSlot, int valuesSlot, int firstClearSlot);
}