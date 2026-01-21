#include "wren.hpp"

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

WREN_CLASS_STATIC("debug", "Profile", "begin(_)", wren_ProfileStaticProfileBegin, "Begin a profile scope.");
WREN_CLASS_STATIC("debug", "Profile", "end()", wren_ProfileStaticProfileEnd, "End a profile scope.");


// Debug.info(string)
void wren_DebugInfo(WrenVM* vm)
{
#ifdef DEBUG
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_INFO(message);
#endif
}

// Debug.warning(string)
void wren_DebugWarning(WrenVM* vm)
{
#ifdef DEBUG
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_INFO(message);
#endif
}

// Debug.error(string)
void wren_DebugError(WrenVM* vm)
{
#ifdef DEBUG
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_INFO(message);
#endif
}

// Debug.fatal(string)
void wren_DebugFatal(WrenVM* vm)
{
#ifdef DEBUG
	const char* message = wrenGetSlotString(vm, 1);
	DEBUG_INFO(message);
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
		DEBUG_FATAL(stack);
		DEBUG_FATAL(message);
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
	DEBUG_FATAL(stack);
	DEBUG_FATAL(message);
	BREAK;
#endif
}

WREN_CLASS_STATIC("debug", "Debug", "info(_)", wren_DebugInfo, "Call Debug Info.");
WREN_CLASS_STATIC("debug", "Debug", "warning(_)", wren_DebugWarning, "Call Debug Warning.");
WREN_CLASS_STATIC("debug", "Debug", "error(_)", wren_DebugError, "Call Debug Error.");
WREN_CLASS_STATIC("debug", "Debug", "fatal(_)", wren_DebugFatal, "Call Debug Fatal.");
WREN_CLASS_STATIC("debug", "Debug", "assert(_)", wren_DebugAssert, "Call Debug Assert.");
WREN_CLASS_STATIC("debug", "Debug", "assertMsg(_,_)", wren_DebugAssertMsg, "Call Debug Assert with Message.");
WREN_CLASS_STATIC("debug", "Debug", "breakpoint()", wren_DebugBreak, "Call Debug Break.");
WREN_CLASS_STATIC("debug", "Debug", "breakpointMsg(_)", wren_DebugBreakMsg, "Call Debug Break with Message.");
