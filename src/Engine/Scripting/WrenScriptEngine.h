#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include "wren.hpp"

#include "WrenBindingRegistry.h"
#include "Debug/Assertions.h"
#include <wren_reflect.h>

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

			static void OnWrenError(WrenVM* vm, WrenErrorType type,
				const char* module, int line, const char* message);

			static void OnWrenWrite(WrenVM* vm, const char* text);

			static WrenLoadModuleResult OnLoadModule(WrenVM* vm, const char* name);

			// Binding callbacks - delegate to registry
			static WrenForeignMethodFn OnBindForeignMethod(
				WrenVM* vm, const char* module, const char* className,
				bool isStatic, const char* signature)
			{
				WrenForeignMethodFn method;
				if (strcmp(module, "reflect") == 0)
				{
					method = wrenReflectBindForeignMethod(vm, className, isStatic, signature);
				}
				else if (!isStatic && strncmp(signature, "init ", 5) == 0)
				{
					method = Wren::FindClass(module, className).allocate;
				}
				else
				{
					method = Wren::FindMethod(module, className, isStatic, signature);
				}

				if (!method)
				{
					DEBUG_WARNING("Wren binding not found: %s.%s.%s (static=%d)",
						module, className, signature, isStatic);
				}

				return method;
			}

			static WrenForeignClassMethods OnBindForeignClass(
				WrenVM* vm, const char* module, const char* className)
			{

				return Wren::FindClass(module, className);
			}

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
