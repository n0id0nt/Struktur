#include "wren.hpp"

#include <string.h>
#include <Trace/wren_trace.h>

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

#include "Debug/Profiling/Profiler.h"
#include "Debug/Assertions.h"

// Profile.begin(name)
void wren_ProfileStaticProfileBegin(WrenVM* vm)
{
#ifdef DEBUG
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	const char* name = wrenGetSlotString(vm, 1);
	context->GetProfiler().BeginProfile(name);
#endif
}

// Profile.end()
void wren_ProfileStaticProfileEnd(WrenVM* vm)
{
#ifdef DEBUG
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	context->GetProfiler().EndProfile();
#endif
}

// Debug.info(string)
void wren_DebugInfo(WrenVM* vm)
{
#ifdef DEBUG
	char stackBuffer[4096];
	const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_INFO("%s\n%s", message, stack);
#endif
}

// Debug.warning(string)
void wren_DebugWarning(WrenVM* vm)
{
#ifdef DEBUG
	char stackBuffer[4096];
	const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_WARNING("%s\n%s", message, stack);
#endif
}

// Debug.error(string)
void wren_DebugError(WrenVM* vm)
{
#ifdef DEBUG
	char stackBuffer[4096];
	const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_ERROR("%s\n%s", message, stack);
#endif
}

// Debug.fatal(string)
void wren_DebugFatal(WrenVM* vm)
{
#ifdef DEBUG
	char stackBuffer[4096];
	const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_FATAL("%s\n%s", message, stack);
#endif
}

// Debug.assert(condition)
void wren_DebugAssert(WrenVM* vm)
{
#ifdef DEBUG
	bool condition = wrenGetSlotBool(vm, 1);
	if (condition)
	{
		char stackBuffer[4096];
		const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
		DEBUG_FATAL(stack);
		BREAK;
		// TODO pause the editor
	}
#endif
}

// Debug.assertMsg(condition, message)
void wren_DebugAssertMsg(WrenVM* vm)
{
#ifdef DEBUG
	bool condition = wrenGetSlotBool(vm, 1);
	if (condition)
	{
		const char* message = wrenGetSlotString(vm, 2);
		char stackBuffer[4096];
		const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
		DEBUG_FATAL("%s\n%s", message, stack);
		BREAK;
		// TODO pause the editor
	}
#endif
}

// Debug.break()
void wren_DebugBreak(WrenVM* vm)
{
#ifdef DEBUG
	char stackBuffer[4096];
	const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
	DEBUG_FATAL(stack);
	BREAK;
#endif
}

// Debug.breakMsg(message)
void wren_DebugBreakMsg(WrenVM* vm)
{
#ifdef DEBUG
	const char* message = wrenGetSlotString(vm, 1);
	char stackBuffer[4096];
	const char* stack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
	DEBUG_FATAL("%s\n%s", message, stack);
	BREAK;
#endif
}

WREN_BINDING_MODULE(Debug)
{
	WREN_CLASS_STATIC(registry, "debug", "Profile", "begin(_)", wren_ProfileStaticProfileBegin, "Begin a profile scope.");
	WREN_CLASS_STATIC(registry, "debug", "Profile", "end()", wren_ProfileStaticProfileEnd, "End a profile scope.");

	WREN_CLASS_STATIC(registry, "debug", "Debug", "info(_)", wren_DebugInfo, "Call Debug Info.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "warning(_)", wren_DebugWarning, "Call Debug Warning.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "error(_)", wren_DebugError, "Call Debug Error.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "fatal(_)", wren_DebugFatal, "Call Debug Fatal.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "assert(_)", wren_DebugAssert, "Call Debug Assert.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "assertMsg(_,_)", wren_DebugAssertMsg, "Call Debug Assert with Message.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "breakpoint()", wren_DebugBreak, "Call Debug Break.");
	WREN_CLASS_STATIC(registry, "debug", "Debug", "breakpointMsg(_)", wren_DebugBreakMsg, "Call Debug Break with Message.");
}
