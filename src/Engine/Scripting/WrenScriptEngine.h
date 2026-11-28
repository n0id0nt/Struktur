#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Debug/Assertions.h"
#include "WrenBindingRegistry.h"
#include "wren.hpp"

namespace Struktur
{
class GameContext;

namespace Wren
{
class WrenScriptEngine
{
public:
	WrenScriptEngine()
	    : m_vm(nullptr)
	{
	}

	// The function that registers every script binding of the game: set once, before Initialise.
	void SetBindings(void (*registerBindings)(BindingRegistry& registry))
	{
		m_registerBindings = registerBindings;
	}
	void Initialise(GameContext& ctx);
	void Shutdown();

	WrenVM* GetVM()
	{
		return m_vm;
	}
	BindingRegistry& GetRegistry()
	{
		return m_registry;
	}

	bool InterpretString(const char* module, const char* source);
	bool InterpretFile(const char* path);

private:
	WrenVM* m_vm;
	void (*m_registerBindings)(BindingRegistry& registry) = nullptr;
	BindingRegistry m_registry;
	// The binding modules' Wren source by module name, built from m_registry. Wren reads these strings directly.
	std::unordered_map<std::string, std::string> m_moduleSources;

	static void OnWrenError(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message);
	static void OnWrenWrite(WrenVM* vm, const char* text);
	static WrenLoadModuleResult OnLoadModule(WrenVM* vm, const char* name);
	static WrenForeignMethodFn OnBindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic,
	                                               const char* signature);
	static WrenForeignClassMethods OnBindForeignClass(WrenVM* vm, const char* module, const char* className);
};
}  // namespace Wren
}  // namespace Struktur
