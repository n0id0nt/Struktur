#pragma once

#include "WrenBindingRegistry.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <filesystem>

#include "Debug/Assertions.h"

namespace Struktur::Wren {

    class CodeGenerator {
    public:
        // Generate all .wren declaration files from registered bindings
        static void GenerateBindingFiles(const std::string& outputDir) {
            // Create output directory if it doesn't exist
            std::filesystem::create_directories(outputDir);
            
            auto& registry = g_methodBindings;
            
            // Group bindings by module
            std::unordered_map<std::string, std::vector<const MethodBinding*>> methodsByModule;
            std::unordered_map<std::string, std::vector<const ClassBinding*>> classesByModule;
            std::unordered_map<std::string, std::vector<const EnumBinding*>> enumsByModule;
            std::unordered_map<std::string, std::vector<const ConstantBinding*>> constantsByModule;
            
            for (const auto& method : g_methodBindings) {
                methodsByModule[method.moduleName].push_back(&method);
            }
            
            for (const auto& cls : g_classBindings) {
                classesByModule[cls.moduleName].push_back(&cls);
            }
            
            for (const auto& enumBinding : g_enumBindings) {
                enumsByModule[enumBinding.moduleName].push_back(&enumBinding);
            }
            
            for (const auto& constant : g_constantBindings) {
                constantsByModule[constant.moduleName].push_back(&constant);
            }
            
            // Generate a .wren file for each module
            for (const auto& [moduleName, methods] : methodsByModule) {
                GenerateModuleFile(
                    outputDir, 
                    moduleName, 
                    methods, 
                    classesByModule[moduleName],
                    enumsByModule[moduleName],
                    constantsByModule[moduleName]
                );
            }
            
            DEBUG_INFO("Generated %zu Wren binding file(s) in: %s", 
                    methodsByModule.size(), outputDir.c_str());
        }
        
    private:
        static void GenerateModuleFile(
            const std::string& outputDir,
            const std::string& moduleName,
            const std::vector<const MethodBinding*>& methods,
            const std::vector<const ClassBinding*>& classes,
            const std::vector<const EnumBinding*>& enums,
            const std::vector<const ConstantBinding*>& constants) {
            
            std::string filePath = outputDir + "/" + moduleName + ".wren";
            std::ofstream file(filePath);
            
            if (!file.is_open()) {
                DEBUG_ERROR("Failed to create file: %s", filePath.c_str());
                return;
            }
            
            file << "// AUTO-GENERATED FILE - DO NOT EDIT\n";
            file << "// Generated from C++ bindings\n";
            file << "// Module: " << moduleName << "\n\n";
            
            // Generate module-level constants first
            bool hasModuleConstants = false;
            for (const auto* constant : constants) {
                if (constant->className.empty()) {
                    if (!hasModuleConstants) {
                        file << "// Module Constants\n";
                        hasModuleConstants = true;
                    }
                    if (!constant->documentation.empty()) {
                        file << "// " << constant->documentation << "\n";
                    }
                    file << "var " << constant->name << " = " << constant->value << "\n";
                }
            }
            if (hasModuleConstants) file << "\n";
            
            // Generate module-level functions
            bool hasModuleFunctions = false;
            for (const auto* method : methods) {
                if (method->className.empty()) {
                    if (!hasModuleFunctions) {
                        file << "// Module Functions\n";
                        hasModuleFunctions = true;
                    }
                    if (!method->documentation.empty()) {
                        file << "// " << method->documentation << "\n";
                    }
                    std::string signature = ConvertSignature(method->signature);
                    file << "foreign " << signature << "\n";
                }
            }
            if (hasModuleFunctions) file << "\n";
            
            // Generate enums
            for (const auto* enumBinding : enums) {
                if (!enumBinding->documentation.empty()) {
                    file << "// " << enumBinding->documentation << "\n";
                }
                file << "class " << enumBinding->enumName << " {\n";
                for (const auto& [name, value] : enumBinding->values) {
                    file << "    static " << name << " { " << value << " }\n";
                }
                file << "}\n\n";
            }
            
            // Group methods by class
            std::unordered_map<std::string, std::vector<const MethodBinding*>> methodsByClass;
            for (const auto* method : methods) {
                if (!method->className.empty()) {
                    methodsByClass[method->className].push_back(method);
                }
            }
            
            // Generate each class
            for (const auto& [className, classMethods] : methodsByClass) {
                // Check if it's a foreign class
                bool isForeignClass = false;
                for (const auto* cls : classes) {
                    if (cls->className == className) {
                        isForeignClass = true;
                        if (!cls->documentation.empty()) {
                            file << "// " << cls->documentation << "\n";
                        }
                        break;
                    }
                }
                
                if (isForeignClass) {
                    file << "foreign class " << className << " {\n";
                } else {
                    file << "class " << className << " {\n";
                }
                
                // Add class constants
                for (const auto* constant : constants) {
                    if (constant->className == className) {
                        if (!constant->documentation.empty()) {
                            file << "    // " << constant->documentation << "\n";
                        }
                        file << "    static " << constant->name << " { " 
                            << constant->value << " }\n";
                    }
                }
                
                // Add methods
                for (const auto* method : classMethods) {
                    // Add documentation as comment
                    if (!method->documentation.empty()) {
                        file << "    // " << method->documentation << "\n";
                    }
                    
                    // Generate method declaration
                    if (method->isStatic) {
                        file << "    foreign static ";
                    } else {
                        file << "    foreign ";
                    }
                    
                    // Convert signature to readable format
                    std::string readableSignature = ConvertSignature(method->signature);
                    file << readableSignature << "\n";
                }
                
                file << "}\n\n";
            }
            
            file.close();
            DEBUG_INFO("Generated: %s", filePath.c_str());
        }
        
        static std::string ConvertSignature(const std::string& signature) {
            // Convert "methodName(_,_)" to "methodName(arg0, arg1)"
            std::string result = signature;
            int argCount = 0;
            
            size_t pos = 0;
            while ((pos = result.find('_', pos)) != std::string::npos) {
                std::string argName = "arg" + std::to_string(argCount++);
                result.replace(pos, 1, argName);
                pos += argName.length();
            }
            
            return result;
        }
    };

}
