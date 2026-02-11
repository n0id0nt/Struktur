#include "DialogueHelperFunctions.h"

std::vector<Struktur::Callback::Variant> Struktur::Dialogue::HelperFunctions::ConvertParamsToVariant(std::unordered_map<std::string, DialogueValue> params)
{
    Struktur::Callback::VariantMap variantMap;

    for (const auto& [key, value] : params)
    {
        Struktur::Callback::Variant variantValue;

        if (value.IsString())       variantValue = value.AsString();
        else if (value.IsInt())     variantValue = value.AsInt();
        else if (value.IsBool())    variantValue = value.AsBool();
        else if (value.IsDouble())  variantValue = value.AsDouble();
        else                        variantValue = nullptr;

        variantMap.items.insert({key, variantValue});
    }

    return { variantMap };
}