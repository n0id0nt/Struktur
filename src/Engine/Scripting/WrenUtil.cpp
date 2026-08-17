#include "WrenUtil.h"

#include "Engine/Scripting/WrenValueWrapper.h"

std::vector<Struktur::Wren::WrenItem> Struktur::Wren::Util::GetWrenMapFromPairs(WrenVM* vm, int pairsSlot,
                                                                                int firstClearSlot)
{
	std::vector<Wren::WrenItem> items;

	int pairCount = wrenGetListCount(vm, pairsSlot);
	int pairSlot  = firstClearSlot;
	int valueSlot = firstClearSlot + 1;
	wrenEnsureSlots(vm, valueSlot + 1);

	// Index 0 is the ["__type", "map"] sentinel written by MapUtil.mapToPairs - the actual entries start at 1
	for (int i = 1; i < pairCount; i++)
	{
		wrenGetListElement(vm, pairsSlot, i, pairSlot);

		Wren::WrenItem item;

		// Get key
		wrenGetListElement(vm, pairSlot, 0, valueSlot);
		item.identifier = wrenGetSlotString(vm, valueSlot);

		// Get value
		wrenGetListElement(vm, pairSlot, 1, valueSlot);
		item.type = wrenGetSlotType(vm, valueSlot);

		switch (item.type)
		{
			case WREN_TYPE_NUM:
				item.value = wrenGetSlotDouble(vm, valueSlot);
				break;

			case WREN_TYPE_BOOL:
				item.value = wrenGetSlotBool(vm, valueSlot);
				break;

			case WREN_TYPE_STRING:
				item.value = std::string(wrenGetSlotString(vm, valueSlot));
				break;

			case WREN_TYPE_NULL:
				item.value = nullptr;
				break;

			default:
				// Skip or handle as needed
				continue;
		}
		items.emplace_back(item);
	}

	return items;
}

Struktur::Wren::WrenItem Struktur::Wren::Util::GetWrenItemFromSlot(WrenVM* vm, int slot)
{
	WrenItem item;
	item.type = wrenGetSlotType(vm, slot);

	switch (item.type)
	{
		case WREN_TYPE_NUM:
			item.value = wrenGetSlotDouble(vm, slot);
			break;

		case WREN_TYPE_BOOL:
			item.value = wrenGetSlotBool(vm, slot);
			break;

		case WREN_TYPE_STRING:
			item.value = std::string(wrenGetSlotString(vm, slot));
			break;

		case WREN_TYPE_NULL:
		default:
			item.value = nullptr;
			break;
	}

	return item;
}

void Struktur::Wren::Util::SetSlotFromWrenItem(WrenVM* vm, int slot, const WrenItem& item)
{
	switch (item.type)
	{
		case WREN_TYPE_NUM:
			wrenSetSlotDouble(vm, slot, std::any_cast<double>(item.value));
			break;

		case WREN_TYPE_BOOL:
			wrenSetSlotBool(vm, slot, std::any_cast<bool>(item.value));
			break;

		case WREN_TYPE_STRING:
			wrenSetSlotString(vm, slot, std::any_cast<std::string>(item.value).c_str());
			break;

		case WREN_TYPE_NULL:
		default:
			wrenSetSlotNull(vm, slot);
			break;
	}
}
