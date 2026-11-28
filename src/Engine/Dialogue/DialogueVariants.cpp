#include "DialogueVariants.h"

namespace Struktur::Dialogue::HelperFunctions
{
DialogueValue ConvertVariantToDialogueValue(Callback::Variant variant)
{
	DialogueValue value;
	std::visit(
	    [&value](auto&& arg)
	    {
		    using T = std::decay_t<decltype(arg)>;

		    if constexpr (std::is_same_v<T, std::string>)
		    {
			    value = DialogueValue(arg.c_str());
		    }
		    else if constexpr (std::is_same_v<T, int>)
		    {
			    value = DialogueValue(static_cast<double>(arg));
		    }
		    else if constexpr (std::is_same_v<T, bool>)
		    {
			    value = DialogueValue(arg);
		    }
		    else if constexpr (std::is_same_v<T, double>)
		    {
			    value = DialogueValue(arg);
		    }
	    },
	    variant);
	return value;
}

std::vector<Callback::Variant> ConvertParamsToVariant(std::unordered_map<std::string, DialogueValue> params)
{
	Callback::VariantMap variantMap;

	for (const auto& [key, value] : params)
	{
		Callback::Variant variantValue;

		if (value.IsString())
		{
			variantValue = value.AsString();
		}
		else if (value.IsInt())
		{
			variantValue = value.AsInt();
		}
		else if (value.IsBool())
		{
			variantValue = value.AsBool();
		}
		else if (value.IsDouble())
		{
			variantValue = value.AsDouble();
		}
		else
		{
			variantValue = nullptr;
		}

		variantMap.items.insert({key, variantValue});
	}

	return {variantMap};
}
}  // namespace Struktur::Dialogue::HelperFunctions
