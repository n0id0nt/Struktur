#pragma once

#include <vector>

#include "Engine/Callback/Variant.h"
#include "wren.hpp"

namespace Struktur::Callback::HelperFunctions
{
// Generate Wren call signature like "call(_,_,_)" based on argument count
std::string MakeWrenCallSignature(size_t argCount);

// Push a variant value to a Wren slot
void VariantToWrenSlot(WrenVM* vm, int slot, const Variant& value);

// Extract a variant value from a Wren slot
Variant WrenSlotToVariant(WrenVM* vm, int slot);
}  // namespace Struktur::Callback::HelperFunctions
