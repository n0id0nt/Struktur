#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "nlohmann/json.hpp"
#include "wren.hpp"

// -- helpers ----------------------------------------------------------------------

// Push a json value onto the Wren stack as a Wren Map, List, or primitive.
// `slot` is the destination slot; scratch slots start at `scratchBase`.
static void pushJsonValue(WrenVM* vm, const nlohmann::json& j, int slot, int scratchBase)
{
	if (j.is_null())
	{
		wrenSetSlotNull(vm, slot);
	}
	else if (j.is_boolean())
	{
		wrenSetSlotBool(vm, slot, j.get<bool>());
	}
	else if (j.is_number_integer())
	{
		wrenSetSlotDouble(vm, slot, static_cast<double>(j.get<int64_t>()));
	}
	else if (j.is_number())
	{
		wrenSetSlotDouble(vm, slot, j.get<double>());
	}
	else if (j.is_string())
	{
		wrenSetSlotString(vm, slot, j.get<std::string>().c_str());
	}
	else if (j.is_array())
	{
		wrenEnsureSlots(vm, scratchBase + 1);
		wrenSetSlotNewList(vm, slot);
		int elemSlot = scratchBase;
		for (auto& elem : j)
		{
			pushJsonValue(vm, elem, elemSlot, scratchBase + 1);
			wrenInsertInList(vm, slot, -1, elemSlot);
		}
	}
	else if (j.is_object())
	{
		wrenEnsureSlots(vm, scratchBase + 2);
		wrenSetSlotNewMap(vm, slot);
		int keySlot = scratchBase;
		int valSlot = scratchBase + 1;
		for (auto& [k, v] : j.items())
		{
			wrenSetSlotString(vm, keySlot, k.c_str());
			pushJsonValue(vm, v, valSlot, scratchBase + 2);
			wrenSetMapValue(vm, slot, keySlot, valSlot);
		}
	}
}

// Read a Wren Map/List/primitive from `slot` into a json value.
static nlohmann::json pullWrenValue(WrenVM* vm, int slot)
{
	switch (wrenGetSlotType(vm, slot))
	{
		case WREN_TYPE_NULL:
			return nullptr;
		case WREN_TYPE_BOOL:
			return wrenGetSlotBool(vm, slot);
		case WREN_TYPE_NUM:
			return wrenGetSlotDouble(vm, slot);
		case WREN_TYPE_STRING:
			return wrenGetSlotString(vm, slot);
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
							nlohmann::json obj = nlohmann::json::object();
							for (int i = 1; i < count; ++i)
							{
								wrenGetListElement(vm, slot, i, elemSlot);
								// Each pair is a 2-element list: [key, value]
								wrenGetListElement(vm, elemSlot, 0, innerSlot);
								std::string key = wrenGetSlotString(vm, innerSlot);
								wrenGetListElement(vm, elemSlot, 1, innerSlot);
								obj[key] = pullWrenValue(vm, innerSlot);  // recurse
							}
							return obj;
						}
					}
				}
			}

			// No sentinel - treat as a plain JSON array
			nlohmann::json arr = nlohmann::json::array();
			for (int i = 0; i < count; ++i)
			{
				wrenGetListElement(vm, slot, i, elemSlot);
				arr.push_back(pullWrenValue(vm, elemSlot));  // recurse
			}
			return arr;
		}
		case WREN_TYPE_MAP:
		{
			BREAK_MSG("Can not serialise. A map must be converted to a pair as keys and values.");
			// Wren has no map-iteration API, so we rely on the caller only
			// passing string-keyed maps (which is always true for save data).
			// Iterate via wrenGetMapContainsKey is unavailable for enumeration,
			// so we ask the caller to encode maps as [[key,val],...] arrays
			// OR you use a known schema. For a general solution, see note below.
			//
			// Simple approach: treat as opaque and return null - override with
			// your own schema-aware version if needed.
			return nullptr;
		}
		default:
			return nullptr;
	}
}

// Json.parse(jsonString) -> Map | List | null
void wren_JsonParse(WrenVM* vm)
{
	const char* str = wrenGetSlotString(vm, 1);

	// Copy the string out before anything else since slots can move
	std::string jsonStr(str);

	nlohmann::json json;
	try
	{
		json = nlohmann::json::parse(jsonStr);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		BREAK_MSG("WREN SERIALISATION: JSON parse error - %s", e.what());
		wrenSetSlotNull(vm, 0);
		return;
	}

	pushJsonValue(vm, json, 0, 1);
}

// Json.stringify(value) -> String
void wren_JsonStringify(WrenVM* vm)
{
	nlohmann::json j = pullWrenValue(vm, 1);
	wrenSetSlotString(vm, 0, j.dump(4).c_str());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

WREN_BINDING_MODULE(Serialisation)
{
	// FileSystem static methods
	WREN_CLASS_STATIC(registry, "serialisation", "Json", "parse(_)", wren_JsonParse,
	                  "Takes a json file as a string and returns a wren map or list");
	WREN_CLASS_STATIC(registry, "serialisation", "Json", "stringify(_)", wren_JsonStringify,
	                  "Takes a wren map or list and returns a json file as a string");
}
