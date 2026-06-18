#pragma once

#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include "wren.hpp"

namespace Struktur::Wren
{
// ============================================================================
// Constructor Parameter Info
// ============================================================================

struct ConstructorSignature
{
	std::string signature;
	std::string documentation;
};

// ============================================================================
// BINDING STRUCTURES
// ============================================================================

struct MethodBinding
{
	std::string moduleName;
	std::string className;
	std::string signature;
	bool isStatic;
	WrenForeignMethodFn function;
	std::string documentation;
};

struct ClassBinding
{
	std::string moduleName;
	std::string className;
	WrenForeignMethodFn allocate;
	WrenFinalizerFn finalize;
	std::vector<ConstructorSignature> constructors;
	std::string documentation;
	std::string parentClassName;
};

struct EnumBinding
{
	std::string moduleName;
	std::string enumName;
	std::vector<std::pair<std::string, int>> values;
	std::string documentation;
};

struct ConstantBinding
{
	std::string moduleName;
	std::string className;
	std::string name;
	double value;
	std::string documentation;
};

struct WrenImplementationBinding
{
    std::string moduleName;
    std::string className;
    std::string wrenSource;
    bool isStatic;
};

// ============================================================================
// REGISTRY - owns all binding data, passed explicitly
// ============================================================================

struct BindingRegistry
{
	std::vector<MethodBinding> methods;
	std::vector<ClassBinding> classes;
	std::vector<EnumBinding> enums;
	std::vector<ConstantBinding> constants;
	std::vector<WrenImplementationBinding> wrenImpls;

	void AddMethod(const char* module, const char* className, const char* signature, bool isStatic,
	               WrenForeignMethodFn func, const char* doc)
	{
		methods.push_back({module, className, signature, isStatic, func, doc});
	}

	void AddClass(const char* module, const char* className, WrenForeignMethodFn alloc, WrenFinalizerFn fin,
	              const char* doc)
	{
		classes.push_back({module, className, alloc, fin, {}, doc, ""});
	}

	void AddConstructor(const char* module, const char* className, const char* signature, WrenForeignMethodFn func,
	                    const char* doc)
	{
		for (auto& binding : classes)
		{
			if (binding.moduleName == module && binding.className == className)
			{
				binding.constructors.push_back({signature, doc});
				break;
			}
		}
		std::string formattedSignature = std::format("init {}", signature);
		methods.push_back({module, className, formattedSignature, false, func, doc});
	}

	void AddEnum(const char* module, const char* enumName, std::initializer_list<std::pair<const char*, int>> vals,
	             const char* doc)
	{
		EnumBinding binding;
		binding.moduleName    = module;
		binding.enumName      = enumName;
		binding.documentation = doc;
		for (const auto& [name, value] : vals)
		{
			binding.values.push_back({name, value});
		}
		enums.push_back(std::move(binding));
	}

	void AddConstant(const char* module, const char* className, const char* name, double value, const char* doc)
	{
		constants.push_back({module, className, name, value, doc});
	}

	void AddInheritance(const char* module, const char* className, const char* parentClassName)
	{
		for (auto& binding : classes)
		{
			if (binding.moduleName == module && binding.className == className)
			{
				binding.parentClassName = parentClassName;
				return;
			}
		}
	}

	void AddWrenImpl(const char* module, const char* className, bool isStatic, const char* wrenSource)
    {
        wrenImpls.push_back({module, className, wrenSource, isStatic});
    }

	WrenForeignMethodFn FindMethod(const char* module, const char* className, bool isStatic,
	                               const char* signature) const
	{
		for (const auto& b : methods)
		{
			if (b.moduleName == module && b.className == className && b.isStatic == isStatic &&
			    b.signature == signature)
			{
				return b.function;
			}
		}
		return nullptr;
	}

	WrenForeignClassMethods FindClass(const char* module, const char* className) const
	{
		WrenForeignClassMethods result = {};
		for (const auto& b : classes)
		{
			if (b.moduleName == module && b.className == className)
			{
				result.allocate = b.allocate;
				result.finalize = b.finalize;
				return result;
			}
		}
		return result;
	}
};

// ============================================================================
// Each binding module implements this signature
// ============================================================================
// Forward declarations - implemented in each WrenBinding*.cpp
void RegisterAnimationBindings(BindingRegistry& registry);
void RegisterGameObjectBindings(BindingRegistry& registry);
void RegisterGameObjectComponentBindings(BindingRegistry& registry);
void RegisterMathBindings(BindingRegistry& registry);
void RegisterPhysicsBindings(BindingRegistry& registry);
void RegisterResourceManagerBindings(BindingRegistry& registry);
void RegisterUIBindings(BindingRegistry& registry);
void RegisterDialogueBindings(BindingRegistry& registry);
void RegisterFunctionCallbackBindings(BindingRegistry& registry);
void RegisterFileSystemBindings(BindingRegistry& registry);
void RegisterFlagsBindings(BindingRegistry& registry);
void RegisterApplicationBindings(BindingRegistry& registry);
void RegisterInputBindings(BindingRegistry& registry);
void RegisterDebugBindings(BindingRegistry& registry);
void RegisterSerialisationBindings(BindingRegistry& registry);
void RegisterVariableSubstitutionBindings(BindingRegistry& registry);
void RegisterEventsBindings(BindingRegistry& registry);

// Call this once at startup
void RegisterAllBindings(BindingRegistry& registry);

}  // namespace Struktur::Wren

// ============================================================================
// REGISTRATION MACROS - now take registry as first arg
// ============================================================================

#define WREN_BINDING_MODULE(ModuleName) \
	void Struktur::Wren::Register##ModuleName##Bindings(Struktur::Wren::BindingRegistry& registry)

#define WREN_CLASS_STATIC(registry, module, cls, sig, func, doc) (registry).AddMethod(module, cls, sig, true, func, doc)

#define WREN_CLASS_METHOD(registry, module, cls, sig, func, doc) \
	(registry).AddMethod(module, cls, sig, false, func, doc)

#define WREN_FUNCTION(registry, module, sig, func, doc) (registry).AddMethod(module, "", sig, true, func, doc)

#define WREN_FOREIGN_CLASS(registry, module, cls, alloc, fin, doc) (registry).AddClass(module, cls, alloc, fin, doc)

#define WREN_CONSTRUCTOR(registry, module, cls, sig, func, doc) (registry).AddConstructor(module, cls, sig, func, doc)

#define WREN_CLASS_INHERITANCE(registry, module, className, parentClassName) \
	(registry).AddInheritance(module, className, parentClassName)

#define WREN_ENUM_PAIR(k, v)    \
	std::pair<const char*, int> \
	{                           \
		k, (int)v               \
	}

#define WREN_ENUM(registry, module, enumName, doc, ...) (registry).AddEnum(module, #enumName, {__VA_ARGS__}, doc)

#define WREN_CLASS_CONSTANT(registry, module, cls, name, value, doc) \
	(registry).AddConstant(module, cls, #name, static_cast<double>(value), doc)

#define WREN_CONSTANT(registry, module, name, value, doc) \
	(registry).AddConstant(module, "", #name, static_cast<double>(value), doc)

#define WREN_VARIABLE_READONLY(registry, module, cls, name, getterFunc, doc) \
	(registry).AddMethod(module, cls, #name, true, getterFunc, doc)

#define WREN_VARIABLE(registry, module, cls, name, getterFunc, setterFunc, doc) \
	(registry).AddMethod(module, cls, #name, true, getterFunc, doc);            \
	(registry).AddMethod(module, cls, #name "=(_)", true, setterFunc, doc " (setter)")
	
#define WREN_IMPL(registry, module, cls, isStatic, wrenSource) \
    (registry).AddWrenImpl(module, cls, isStatic, wrenSource)
