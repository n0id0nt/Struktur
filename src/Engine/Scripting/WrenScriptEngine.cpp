#include "WrenScriptEngine.h"

#include <filesystem>

#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Reflect/wren_reflect.h"
#include "Trace/wren_trace.h"
#ifndef DEBUG
	#include "WrenBindings/Bindings/Generated/WrenBindingSources.h"
#endif

void Struktur::Wren::WrenScriptEngine::Initialise(GameContext& context)
{
	// Register all bindings explicitly
	RegisterAllBindings(m_registry);

	WrenConfiguration config;
	wrenInitConfiguration(&config);

	config.errorFn             = OnWrenError;
	config.writeFn             = OnWrenWrite;
	config.loadModuleFn        = OnLoadModule;
	config.bindForeignMethodFn = OnBindForeignMethod;
	config.bindForeignClassFn  = OnBindForeignClass;

	m_vm = wrenNewVM(&config);
	wrenSetUserData(m_vm, &context);

	DEBUG_INFO("Wren VM initialised");
	DEBUG_INFO("Registered %zu method bindings", m_registry.methods.size());
	DEBUG_INFO("Registered %zu class bindings", m_registry.classes.size());
	DEBUG_INFO("Registered %zu enum bindings", m_registry.enums.size());
	DEBUG_INFO("Registered %zu constant bindings", m_registry.constants.size());
}

void Struktur::Wren::WrenScriptEngine::Shutdown()
{
	if (m_vm)
	{
		wrenFreeVM(m_vm);
		m_vm = nullptr;
	}
}

bool Struktur::Wren::WrenScriptEngine::InterpretString(const char* module, const char* source)
{
	if (!m_vm)
	{
		DEBUG_ERROR("Wren VM not initialised");
		return false;
	}

	WrenInterpretResult result = wrenInterpret(m_vm, module, source);

	if (result == WREN_RESULT_COMPILE_ERROR)
	{
		DEBUG_ERROR("Wren compile error in module: %s", module);
		return false;
	}
	else if (result == WREN_RESULT_RUNTIME_ERROR)
	{
		DEBUG_ERROR("Wren runtime error in module: %s", module);
		return false;
	}
	else if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("Wren error in module: %s", module);
		return false;
	}

	return true;
}

bool Struktur::Wren::WrenScriptEngine::InterpretFile(const char* path)
{
	if (!m_vm)
	{
		DEBUG_ERROR("Wren VM not initialised");
		return false;
	}

	auto result = FileSystem::ReadString(path);

	if (!result)
	{
		DEBUG_ERROR("Failed to open Wren file: %s, Error: %s", path, result.errorMessage.c_str());
		return false;
	}

	std::filesystem::path filepath(path);
	std::string module = filepath.stem().string();

	return InterpretString(module.c_str(), result.value.c_str());
}

// ============================================================================
// CALLBACK IMPLEMENTATIONS
// ============================================================================

void Struktur::Wren::WrenScriptEngine::OnWrenError(WrenVM* vm, WrenErrorType type, const char* module, int line,
                                                   const char* message)
{
	switch (type)
	{
		case WREN_ERROR_COMPILE:
			DEBUG_ERROR("[Wren Compile] %s:%d: %s", module, line, message);
			break;
		case WREN_ERROR_RUNTIME:
			DEBUG_ERROR("[Wren Runtime] %s", message);
			break;
		case WREN_ERROR_STACK_TRACE:
			DEBUG_ERROR("[Wren Trace] %s:%d: %s", module, line, message);
			break;
	}
}

void Struktur::Wren::WrenScriptEngine::OnWrenWrite(WrenVM* vm, const char* text)
{
	if (strcmp(text, "\n") == 0)
	{
		return;
	}
	DEBUG_INFO("[Wren Print] %s", text);
}

WrenLoadModuleResult Struktur::Wren::WrenScriptEngine::OnLoadModule(WrenVM* vm, const char* name)
{
	WrenLoadModuleResult result = {};

	if (strcmp(name, "meta") == 0 || strcmp(name, "random") == 0)
	{
		result.source = nullptr;
		return result;
	}

	if (strcmp(name, "reflect") == 0)
	{
		result.source     = wrenReflectSource();
		result.onComplete = nullptr;
		return result;
	}

	if (strcmp(name, "trace") == 0)
	{
		result.source     = wrenTraceSource();
		result.onComplete = nullptr;
		return result;
	}

	std::string source;
	bool found = false;

#ifndef DEBUG
	{
		const auto& sources = GetWrenBindingSources();
		auto it             = sources.find(name);
		if (it != sources.end())
		{
			result.source     = it->second();
			result.onComplete = nullptr;
			return result;
		}
	}
#else
	{
		std::string searchPath = FileSystem::GetWorkingDirectory() + "/../src/WrenBindings/Bindings/" + name + ".wren";
		std::ifstream file(searchPath);
		if (file)
		{
			std::stringstream buffer;
			buffer << file.rdbuf();
			source = buffer.str();
			found  = true;
			DEBUG_INFO("Loaded Wren module: %s from %s", name, searchPath.c_str());
		}
	}

	if (!found)
#endif
	{
		std::string searchPath = std::string("Scripts/") + name + ".wren";
		auto fsResult          = FileSystem::ReadString(searchPath);
		if (fsResult)
		{
			source = std::move(fsResult.value);
			found  = true;
			DEBUG_INFO("Loaded Wren module: %s from %s", name, searchPath.c_str());
		}
	}

	if (!found)
	{
		DEBUG_ERROR("Failed to load Wren module: %s", name);
		return result;
	}

	char* source_copy = new char[source.size() + 1];
	strcpy(source_copy, source.c_str());

	result.source     = source_copy;
	result.onComplete = [](WrenVM* vm, const char* name, WrenLoadModuleResult result) { delete[] result.source; };

	return result;
}

WrenForeignMethodFn Struktur::Wren::WrenScriptEngine::OnBindForeignMethod(WrenVM* vm, const char* module,
                                                                          const char* className, bool isStatic,
                                                                          const char* signature)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& scriptEngine             = context->GetWrenScriptEngine();
	auto& registry                 = scriptEngine.GetRegistry();

	WrenForeignMethodFn method = nullptr;

	if (strcmp(module, "reflect") == 0)
	{
		method = wrenReflectBindForeignMethod(vm, className, isStatic, signature);
	}
	else if (strcmp(module, "trace") == 0)
	{
		method = wrenTraceBindForeignMethod(vm, className, isStatic, signature);
	}
	else
	{
		method = registry.FindMethod(module, className, isStatic, signature);
	}

	// Check for inherited methods - walks the full ancestor chain (not just one level), since a foreign class can
	// itself inherit from another foreign class (see UIScroll : UIClip : UIElement) rather than only ever
	// inheriting directly from a class that already has every method natively registered on it.
	std::string currentClassName = className;
	while (!method)
	{
		std::string parentClassName;
		for (const auto& binding : registry.classes)
		{
			if (binding.moduleName == module && binding.className == currentClassName)
			{
				parentClassName = binding.parentClassName;
				break;
			}
		}
		if (parentClassName.empty())
		{
			break;
		}
		method          = registry.FindMethod(module, parentClassName.c_str(), isStatic, signature);
		currentClassName = parentClassName;
	}

	if (!method)
	{
		DEBUG_WARNING("Wren binding not found: %s.%s.%s (static=%d)", module, className, signature, isStatic);
	}

	return method;
}

WrenForeignClassMethods Struktur::Wren::WrenScriptEngine::OnBindForeignClass(WrenVM* vm, const char* module,
                                                                             const char* className)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& scriptEngine             = context->GetWrenScriptEngine();
	auto& registry                 = scriptEngine.GetRegistry();
	return registry.FindClass(module, className);
}
