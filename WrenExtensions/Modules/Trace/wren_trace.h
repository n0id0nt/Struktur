#ifndef wren_trace_h
#define wren_trace_h

#include "wren.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WREN_OPT_TRACE
#define WREN_OPT_TRACE 1
#endif

#if WREN_OPT_TRACE

	// Returns the Wren source code for the trace module
	const char* wrenTraceSource();
	
	// Returns the current call stack as a formatted string
	// Helper function that builds and returns the call stack string
	const char* wrenTraceGetCallStackString(WrenVM* vm, char* buffer, size_t bufferSize);

	// Binds foreign methods for the trace module
	WrenForeignMethodFn wrenTraceBindForeignMethod(WrenVM* vm,
		const char* className,
		bool isStatic,
		const char* signature);

#endif

#ifdef __cplusplus
}
#endif

#endif
