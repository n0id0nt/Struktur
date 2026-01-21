#ifndef wren_reflect_h
#define wren_reflect_h

#include "wren.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WREN_OPT_REFLECT
#define WREN_OPT_REFLECT 1
#endif

#if WREN_OPT_REFLECT

	// Returns the Wren source code for the reflect module
	const char* wrenReflectSource();

	// Binds foreign methods for the reflect module
	WrenForeignMethodFn wrenReflectBindForeignMethod(WrenVM* vm,
		const char* className,
		bool isStatic,
		const char* signature);

#endif

#ifdef __cplusplus
}
#endif

#endif
