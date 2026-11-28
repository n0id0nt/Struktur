#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{
// Writes the Wren source of the modules scripts import ("math", "input", ...) from the registered bindings.
class CodeGenerator
{
public:
	// Module name -> source, for every module with a binding of any kind.
	static std::unordered_map<std::string, std::string> BuildModuleSources(const BindingRegistry& registry);

private:
	static std::string BuildModuleSource(const std::string& moduleName,
	                                     const std::vector<const MethodBinding*>& methods,
	                                     const std::vector<const ClassBinding*>& classes,
	                                     const std::vector<const EnumBinding*>& enums,
	                                     const std::vector<const ConstantBinding*>& constants,
	                                     const std::vector<const WrenImplementationBinding*>& wrenImpls,
	                                     const std::vector<const ModuleImportBinding*>& imports);

	static std::string ConvertSignature(const std::string& signature);
};
}  // namespace Struktur::Wren
