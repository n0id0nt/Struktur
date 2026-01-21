#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include "wren.hpp"

#include "WrenBindingRegistry.h"
#include "Debug/Assertions.h"

namespace Struktur
{

	class GameContext;

	namespace Wren
	{
		class WrenScriptEngine
		{
		private:
			WrenVM* m_vm;

			// ========================================================================
			// CALLBACKS
			// ========================================================================

			static void OnWrenError(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message);
			static void OnWrenWrite(WrenVM* vm, const char* text);
			static WrenLoadModuleResult OnLoadModule(WrenVM* vm, const char* name);

			// Binding callbacks - delegate to registry
			static WrenForeignMethodFn OnBindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature);
			static WrenForeignClassMethods OnBindForeignClass(WrenVM* vm, const char* module, const char* className);

		public:
			WrenScriptEngine() : m_vm(nullptr) {}

			void Initialise(GameContext& ctx);
			void Shutdown();

			WrenVM* GetVM() { return m_vm; }
			//GameContext* GetContext() { return context; }

			// Compile and run a Wren script
			bool InterpretString(const char* module, const char* source);

			// Load and run a Wren file
			bool InterpretFile(const char* path);
		};
	}
}
