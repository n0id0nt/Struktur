#pragma once

#include <vector>

#include "Variant.h"
#include "wren.hpp"

namespace Struktur::Callback::HelperFunctions
{
// Generate Wren call signature like "call(_,_,_)" based on argument count
std::string MakeWrenCallSignature(size_t argCount);

// Push a variant value to a Wren slot
void VariantToWrenSlot(WrenVM* vm, int slot, const Variant& value);

// Extract a variant value from a Wren slot
Variant WrenSlotToVariant(WrenVM* vm, int slot);

// Convert variadic arguments to vector of variants
template <typename... Args>
std::vector<Variant> ToVariants(Args&&... args)
{
	std::vector<Variant> result;
	result.reserve(sizeof...(Args));
	(result.push_back(Variant(std::forward<Args>(args))), ...);
	return result;
}
}  // namespace Struktur::Callback::HelperFunctions
