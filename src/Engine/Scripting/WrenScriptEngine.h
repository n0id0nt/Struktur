#pragma once

#include <fstream>
#include <sstream>
#include <string>

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
	BindingRegistry m_registry;

	static void OnWrenError(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message);
	static void OnWrenWrite(WrenVM* vm, const char* text);
	static WrenLoadModuleResult OnLoadModule(WrenVM* vm, const char* name);
	static WrenForeignMethodFn OnBindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic,
	                                               const char* signature);
	static WrenForeignClassMethods OnBindForeignClass(WrenVM* vm, const char* module, const char* className);
};
}  // namespace Wren
}  // namespace Struktur
