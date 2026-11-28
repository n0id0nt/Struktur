#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Callback/Variant.h"
#include "Engine/Dialogue/DialogueValue.h"

// Dialogue values to and from the engine's callback arguments.
namespace Struktur::Dialogue::HelperFunctions
{
DialogueValue ConvertVariantToDialogueValue(Callback::Variant variant);
std::vector<Callback::Variant> ConvertParamsToVariant(std::unordered_map<std::string, DialogueValue> params);
}  // namespace Struktur::Dialogue::HelperFunctions
