#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{

    // ============================================================================
    // DEFINE GLOBAL REGISTRIES
    // ============================================================================

    inline std::vector<MethodBinding>& GetMethodBindings()
    {
        static std::vector<MethodBinding> bindings;
        return bindings;
    }
    inline std::vector<ClassBinding>& GetClassBindings()
    {
        static std::vector<ClassBinding> bindings;
        return bindings;
    }
    inline std::vector<EnumBinding>& GetEnumBindings()
    {
        static std::vector<EnumBinding> bindings;
        return bindings;
    }
    inline std::vector<ConstantBinding>& GetConstantBindings()
    {
        static std::vector<ConstantBinding> bindings;
        return bindings;
    }

    // ============================================================================
    // LOOKUP IMPLEMENTATIONS
    // ============================================================================

    WrenForeignMethodFn FindMethod(const char* module, const char* className, bool isStatic, const char* signature)
    {
        for (const auto& binding : GetMethodBindings())
        {
            if (binding.moduleName == module &&
                binding.className == className &&
                binding.isStatic == isStatic &&
                binding.signature == signature)
            {
                return binding.function;
            }
        }
        return nullptr;
    }

    WrenForeignClassMethods FindClass(const char* module, const char* className)
    {
        WrenForeignClassMethods methods = {};
        for (const auto& binding : GetClassBindings())
        {
            if (binding.moduleName == module && binding.className == className)
            {
                methods.allocate = binding.allocate;
                methods.finalize = binding.finalize;
                return methods;
            }
        }
        return methods;
    }

} // namespace Struktur::Wren
