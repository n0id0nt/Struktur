#ifndef wren_trace_h
#define wren_trace_h

// wren.h itself declares the Wren C API with no extern "C" guard (see wren.hpp, the C++-safe wrapper around it).
// Using the raw header here meant whichever of this file's own C++ #includers happened to pull in "wren.h"
// unwrapped first (via this file, before reaching any of their own "wren.hpp" includes) would silently poison the
// header guard, leaving every subsequent Wren API call in that translation unit looking for a C++-mangled symbol
// that only exists with C linkage - order-dependent and exactly the kind of thing an automated include-sorter can
// (and did) break by innocently reordering unrelated #include lines. Using wren.hpp here whenever this header is
// itself compiled as C++ removes the ordering dependency entirely, matching the __cplusplus check already used
// below for this file's own declarations. wren_trace.c (compiled as plain C) still gets raw wren.h, since
// extern "C" isn't valid C syntax.
#ifdef __cplusplus
#include "wren.hpp"
#else
#include "wren.h"
#endif

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
