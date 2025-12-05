#pragma once

#include "wren.hpp"
#include <vector>
#include <string>
#include <unordered_map>

namespace Struktur::Wren
{

    // ============================================================================
    // BINDING STRUCTURES
    // ============================================================================

    struct MethodBinding {
        std::string moduleName;
        std::string className;
        std::string signature;
        bool isStatic;
        WrenForeignMethodFn function;
        std::string documentation;
    };

    struct ClassBinding {
        std::string moduleName;
        std::string className;
        WrenForeignMethodFn allocate;
        WrenFinalizerFn finalize;
        std::string documentation;
    };

    struct EnumBinding {
        std::string moduleName;
        std::string enumName;
        std::vector<std::pair<std::string, int>> values;
        std::string documentation;
    };

    struct ConstantBinding {
        std::string moduleName;
        std::string className;  // Empty for module-level constants
        std::string name;
        double value;  // Wren only has number type
        std::string documentation;
    };

    // ============================================================================
    // GLOBAL REGISTRIES
    // ============================================================================

    extern std::vector<MethodBinding>& GetMethodBindings();
    extern std::vector<ClassBinding>& GetClassBindings();
    extern std::vector<EnumBinding>& GetEnumBindings();
    extern std::vector<ConstantBinding>& GetConstantBindings();

    // ============================================================================
    // REGISTRAR HELPERS (Static initialization)
    // ============================================================================

    struct MethodRegistrar
    {
        MethodRegistrar(const char* module, const char* className, const char* signature,
                        bool isStatic, WrenForeignMethodFn func, const char* doc)
        {
            GetMethodBindings().push_back({module, className, signature, isStatic, func, doc});
        }
    };

    struct ClassRegistrar
    {
        ClassRegistrar(const char* module, const char* className,
                    WrenForeignMethodFn alloc, WrenFinalizerFn fin, const char* doc)
        {
            GetClassBindings().push_back({module, className, alloc, fin, doc});
        }
    };

    struct EnumRegistrar
    {
        EnumRegistrar(const char* module, const char* enumName,
                    std::initializer_list<std::pair<const char*, int>> values, const char* doc)
        {
            EnumBinding binding;
            binding.moduleName = module;
            binding.enumName = enumName;
            binding.documentation = doc;
            for (const auto& [name, value] : values)
            {
                binding.values.push_back({name, value});
            }
            GetEnumBindings().push_back(binding);
        }
    };

    struct ConstantRegistrar
    {
        ConstantRegistrar(const char* module, const char* className, 
                        const char* name, double value, const char* doc)
        {
            GetConstantBindings().push_back({module, className, name, value, doc});
        }
    };

    // ============================================================================
    // LOOKUP FUNCTIONS
    // ============================================================================

    WrenForeignMethodFn FindMethod(const char* module, const char* className,
                                bool isStatic, const char* signature);
    WrenForeignClassMethods FindClass(const char* module, const char* className);

}

// ============================================================================
// REGISTRATION MACROS
// ============================================================================

// Class static method: Entity.create(name, parent)
#define WREN_CLASS_STATIC(module, cls, sig, func, doc) \
    static Struktur::Wren::MethodRegistrar _wren_reg_##func( \
        module, cls, sig, true, func, doc)

// Class instance method: vector.normalize()
#define WREN_CLASS_METHOD(module, cls, sig, func, doc) \
    static Struktur::Wren::MethodRegistrar _wren_reg_##func( \
        module, cls, sig, false, func, doc)

// Module-level function: lerp(a, b, t)
#define WREN_FUNCTION(module, sig, func, doc) \
    static Struktur::Wren::MethodRegistrar _wren_reg_##func( \
        module, "", sig, true, func, doc)

// Foreign class with allocator and finalizer
#define WREN_FOREIGN_CLASS(module, cls, alloc, fin, doc) \
    static Struktur::Wren::ClassRegistrar _wren_cls_reg_##alloc( \
        module, cls, alloc, fin, doc)

// Enum binding: CollisionType { None = 0, Wall = 1, Enemy = 2 }
#define WREN_ENUM(module, enumName, values, doc) \
    static Struktur::Wren::EnumRegistrar _wren_enum_##enumName( \
        module, #enumName, values, doc)

// Class constant: Math.PI = 3.14159
#define WREN_CLASS_CONSTANT(module, cls, name, value, doc) \
    static Struktur::Wren::ConstantRegistrar _wren_const_##name( \
        module, cls, #name, static_cast<double>(value), doc)

// Module-level constant: PI = 3.14159
#define WREN_CONSTANT(module, name, value, doc) \
    static Struktur::Wren::ConstantRegistrar _wren_const_##name( \
        module, "", #name, static_cast<double>(value), doc)

// Read-only variable (getter only): Time.delta
#define WREN_VARIABLE_READONLY(module, cls, name, getterFunc, doc) \
    static Struktur::Wren::MethodRegistrar _wren_var_get_##getterFunc( \
        module, cls, #name, true, getterFunc, doc)

// Read-write variable (getter + setter): Game.speed
#define WREN_VARIABLE(module, cls, name, getterFunc, setterFunc, doc) \
    static Struktur::Wren::MethodRegistrar _wren_var_get_##getterFunc( \
        module, cls, #name, true, getterFunc, doc); \
    static Struktur::Wren::MethodRegistrar _wren_var_set_##setterFunc( \
        module, cls, #name "=(_)", true, setterFunc, doc " (setter)")
