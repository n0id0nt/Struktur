#include "CallbackHelperFunctions.h"

#include "Debug/Assertions.h"
#include "Engine/UI/UIElement.h"
#include "WrenBindings/WrenMath.h"
#include "WrenBindings/WrenUI.h"

std::string Struktur::Callback::HelperFunctions::MakeWrenCallSignature(size_t argCount)
{
	if (argCount == 0)
	{
		return "call()";
	}

	std::string sig = "call(";
	for (size_t i = 0; i < argCount; ++i)
	{
		if (i > 0)
		{
			sig += ",";
		}
		sig += "_";
	}
	sig += ")";
	return sig;
}

void Struktur::Callback::HelperFunctions::VariantToWrenSlot(WrenVM* vm, int slot, const Variant& value)
{
	std::visit(
	    [vm, slot](auto&& arg)
	    {
		    using T = std::decay_t<decltype(arg)>;

		    if constexpr (std::is_same_v<T, std::nullptr_t>)
		    {
			    wrenSetSlotNull(vm, slot);
		    }
		    else if constexpr (std::is_same_v<T, bool>)
		    {
			    wrenSetSlotBool(vm, slot, arg);
		    }
		    else if constexpr (std::is_same_v<T, int>)
		    {
			    wrenSetSlotDouble(vm, slot, static_cast<double>(arg));
		    }
		    else if constexpr (std::is_same_v<T, float>)
		    {
			    wrenSetSlotDouble(vm, slot, static_cast<double>(arg));
		    }
		    else if constexpr (std::is_same_v<T, double>)
		    {
			    wrenSetSlotDouble(vm, slot, arg);
		    }
		    else if constexpr (std::is_same_v<T, std::string>)
		    {
			    wrenSetSlotString(vm, slot, arg.c_str());
		    }
		    else if constexpr (std::is_same_v<T, glm::vec2>)
		    {
			    wrenEnsureSlots(vm, slot + 2);
			    wrenGetVariable(vm, "math", "Vec2", slot + 1);
			    WrenVec2* foreign = static_cast<WrenVec2*>(wrenSetSlotNewForeign(vm, slot, slot + 1, sizeof(WrenVec2)));
			    new (foreign) WrenVec2(arg);
		    }
		    else if constexpr (std::is_same_v<T, glm::vec3>)
		    {
			    wrenEnsureSlots(vm, slot + 2);
			    wrenGetVariable(vm, "math", "Vec3", slot + 1);
			    WrenVec3* foreign = static_cast<WrenVec3*>(wrenSetSlotNewForeign(vm, slot, slot + 1, sizeof(WrenVec3)));
			    new (foreign) WrenVec3(arg);
		    }
		    else if constexpr (std::is_same_v<T, glm::vec4>)
		    {
			    wrenEnsureSlots(vm, slot + 2);
			    wrenGetVariable(vm, "math", "Vec4", slot + 1);
			    WrenVec4* foreign = static_cast<WrenVec4*>(wrenSetSlotNewForeign(vm, slot, slot + 1, sizeof(WrenVec4)));
			    new (foreign) WrenVec4(arg);
		    }
		    else if constexpr (std::is_same_v<T, UI::UIElement*>)
		    {
			    wrenEnsureSlots(vm, slot + 2);
			    wrenGetVariable(vm, "ui", "UIElement", slot + 1);
			    WrenUIElement* foreign =
			        static_cast<WrenUIElement*>(wrenSetSlotNewForeign(vm, slot, slot + 1, sizeof(WrenUIElement)));
			    new (foreign) WrenUIElement{arg, false};
		    }
		    else if constexpr (std::is_same_v<T, VariantList>)
		    {
			    wrenEnsureSlots(vm, slot + 2);
			    wrenSetSlotNewList(vm, slot);

			    int elementSlot = slot + 1;
			    for (const auto& item : arg.items)
			    {
				    VariantToWrenSlot(vm, elementSlot, item);
				    wrenInsertInList(vm, slot, -1, elementSlot);
			    }
		    }
		    else if constexpr (std::is_same_v<T, VariantMap>)
		    {
			    wrenEnsureSlots(vm, slot + 3);
			    wrenSetSlotNewMap(vm, slot);

			    int keySlot   = slot + 1;
			    int valueSlot = slot + 2;
			    for (const auto& [key, val] : arg.items)
			    {
				    wrenSetSlotString(vm, keySlot, key.c_str());
				    VariantToWrenSlot(vm, valueSlot, val);
				    wrenSetMapValue(vm, slot, keySlot, valueSlot);
			    }
		    }
	    },
	    value);
}

Struktur::Callback::Variant Struktur::Callback::HelperFunctions::WrenSlotToVariant(WrenVM* vm, int slot)
{
	WrenType type = wrenGetSlotType(vm, slot);

	switch (type)
	{
		case WREN_TYPE_BOOL:
			return Variant(wrenGetSlotBool(vm, slot));

		case WREN_TYPE_NUM:
			return Variant(wrenGetSlotDouble(vm, slot));

		case WREN_TYPE_STRING:
			return Variant(std::string(wrenGetSlotString(vm, slot)));

		case WREN_TYPE_LIST:
		{
			int count = wrenGetListCount(vm, slot);
			wrenEnsureSlots(vm, slot + 3);
			int elemSlot  = slot + 1;
			int innerSlot = slot + 2;

			// Check for the ["__type", "map"] sentinel at index 0
			if (count > 0)
			{
				wrenGetListElement(vm, slot, 0, elemSlot);
				if (wrenGetSlotType(vm, elemSlot) == WREN_TYPE_LIST && wrenGetListCount(vm, elemSlot) == 2)
				{
					wrenGetListElement(vm, elemSlot, 0, innerSlot);
					if (wrenGetSlotType(vm, innerSlot) == WREN_TYPE_STRING &&
					    std::string(wrenGetSlotString(vm, innerSlot)) == "__type")
					{
						wrenGetListElement(vm, elemSlot, 1, innerSlot);
						if (wrenGetSlotType(vm, innerSlot) == WREN_TYPE_STRING &&
						    std::string(wrenGetSlotString(vm, innerSlot)) == "map")
						{
							// It's an encoded map - pairs start at index 1
							VariantMap map;
							for (int i = 1; i < count; ++i)
							{
								wrenGetListElement(vm, slot, i, elemSlot);
								// Each pair is a 2-element list: [key, value]
								wrenGetListElement(vm, elemSlot, 0, innerSlot);
								std::string key = wrenGetSlotString(vm, innerSlot);
								wrenGetListElement(vm, elemSlot, 1, innerSlot);
								map.items.emplace(key, WrenSlotToVariant(vm, innerSlot));  // recurse
							}

							return Variant(map);
						}
					}
				}
			}

			VariantList list;
			list.items.reserve(count);

			wrenEnsureSlots(vm, slot + 2);
			int elementSlot = slot + 1;

			for (int i = 0; i < count; ++i)
			{
				wrenGetListElement(vm, slot, i, elementSlot);
				list.items.push_back(WrenSlotToVariant(vm, elementSlot));
			}

			return Variant(list);
		}

		case WREN_TYPE_MAP:
		{
			VariantMap map;

			// Note: Wren's C API doesn't provide map iteration
			// You need to either:
			// 1. Use known keys and wrenGetMapValue for each
			// 2. Create a Wren-side helper to convert map to list of [key, value] pairs
			// 3. Accept that maps can only go C++ -> Wren, not Wren -> C++

			BREAK_MSG("WrenSlotToVariant: Map extraction not fully supported by Wren C API");
			return Variant(map);
		}

		case WREN_TYPE_FOREIGN:
		{
			// Extract foreign objects - in practice you need to know what type to expect
			// For now, try to extract as glm types
			void* foreign = wrenGetSlotForeign(vm, slot);

			// Note: Wren doesn't provide type checking for foreign objects
			// You'd need to track which class a foreign is via your own metadata
			// For simplicity, we'll just return null for foreign types
			// In real usage, you'd check context to know which foreign type to extract

			BREAK_MSG("WrenSlotToVariant: Foreign object extraction requires type context");
			return Variant(nullptr);
		}

		case WREN_TYPE_NULL:
		default:
			return Variant(nullptr);
	}
}
