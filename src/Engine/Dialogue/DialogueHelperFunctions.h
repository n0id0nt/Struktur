#pragma once

#include <unordered_map>
#include <vector>

#include "Engine/Callback/Variant.h"
#include "DialogueValue.h"

namespace Struktur::Dialogue::HelperFunctions
{
    std::vector<Struktur::Callback::Variant> ConvertParamsToVariant(std::unordered_map<std::string, DialogueValue> params);
}