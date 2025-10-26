#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{

    // ============================================================================
    // DEFINE GLOBAL REGISTRIES
    // ============================================================================

    std::vector<MethodBinding> g_methodBindings;
    std::vector<ClassBinding> g_classBindings;
    std::vector<EnumBinding> g_enumBindings;
    std::vector<ConstantBinding> g_constantBindings;

    // ============================================================================
    // LOOKUP IMPLEMENTATIONS
    // ============================================================================

    WrenForeignMethodFn FindMethod(const char* module, const char* className,
                                bool isStatic, const char* signature)
    {
        for (const auto& binding : g_methodBindings) {
            if (binding.moduleName == module &&
                binding.className == className &&
                binding.isStatic == isStatic &&
                binding.signature == signature) {
                return binding.function;
            }
        }
        return nullptr;
    }

    WrenForeignClassMethods FindClass(const char* module, const char* className)
    {
        WrenForeignClassMethods methods = {};
        for (const auto& binding : g_classBindings)
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

    WrenForeignClassMethods FindClass(const char* module, const char* className)
    {
        WrenForeignClassMethods methods = {};
        for (const auto& binding : g_classBindings)
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
