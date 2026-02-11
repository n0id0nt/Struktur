#include "CallbackHelperFunctions.h"

#include "Debug/Assertions.h"

std::string Struktur::Callback::HelperFunctions::MakeWrenCallSignature(size_t argCount)
{
    if (argCount == 0)
    {
        return "call()";
    }
    
    std::string sig = "call(";
    for (size_t i = 0; i < argCount; ++i)
    {
        if (i > 0) sig += ",";
        sig += "_";
    }
    sig += ")";
    return sig;
}

void Struktur::Callback::HelperFunctions::VariantToWrenSlot(WrenVM *vm, int slot, const Variant &value)
{
    std::visit([vm, slot](auto&& arg)
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
    }, value);
}

Struktur::Callback::Variant Struktur::Callback::HelperFunctions::WrenSlotToVariant(WrenVM *vm, int slot)
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
            
        case WREN_TYPE_NULL:
        default:
            return Variant(nullptr);
    }
}
