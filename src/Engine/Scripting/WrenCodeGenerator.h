#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "Debug/Assertions.h"
#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{
class CodeGenerator
{
   public:
	static void GenerateBindingFiles(const BindingRegistry& registry, const std::string& outputDir);

   private:
	static void GenerateModuleFile(const std::string& outputDir, const std::string& moduleName,
	                               const std::vector<const MethodBinding*>& methods,
	                               const std::vector<const ClassBinding*>& classes,
	                               const std::vector<const EnumBinding*>& enums,
	                               const std::vector<const ConstantBinding*>& constants,
								   const std::vector<const WrenImplementationBinding*>& wrenImpls,
								   const std::vector<const ModuleImportBinding*>& imports);

	static std::string ConvertSignature(const std::string& signature);

	static void GenerateAllHeaders(const std::string& wrenSourceDir, const std::string& headerOutputDir);
};
}  // namespace Struktur::Wren
