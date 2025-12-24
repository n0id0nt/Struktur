#include "WrenUtil.h"

#include "Engine/Scripting/WrenValueWrapper.h"
#include "Debug/Assertions.h"

std::vector<Struktur::Wren::WrenItem> Struktur::Wren::Util::GetWrenMapDoubleList(WrenVM *vm, int keysSlot, int valuesSlot, int firstClearSlot)
{
    std::vector<Wren::WrenItem> items;

    int itemKeyCount = wrenGetListCount(vm, keysSlot);
	int itemValueCount = wrenGetListCount(vm, valuesSlot);
	ASSERT_MSG(itemKeyCount == itemValueCount, "The item key list and item value list must be the same");

    for (int i = 0; i < itemKeyCount; i++)
    {
        Wren::WrenItem item;

        // Get key
        wrenGetListElement(vm, keysSlot, i, firstClearSlot);
        item.identifier = wrenGetSlotString(vm, firstClearSlot);

        // Get value
        wrenGetListElement(vm, valuesSlot, i, valuesSlot + 1);
        item.type = wrenGetSlotType(vm, valuesSlot + 1);

        switch (item.type)
        {
        case WREN_TYPE_NUM:
            item.value = wrenGetSlotDouble(vm, valuesSlot + 1);
            break;
        
        case WREN_TYPE_BOOL:
            item.value = wrenGetSlotBool(vm, valuesSlot + 1);
            break;
            
        case WREN_TYPE_STRING:
            item.value = std::string(wrenGetSlotString(vm, valuesSlot + 1));
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
