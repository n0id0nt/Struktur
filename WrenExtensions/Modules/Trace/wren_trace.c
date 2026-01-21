#include "wren_trace.h"
#include "wren_vm.h"
#include "wren_value.h"
#include "wren_common.h"

#include <string.h>
#include "wren_trace.wren.inc"

// The Wren source code (can be auto-generated from .wren file)
const char* wrenTraceSource()
{
	return traceModuleSource;
}

// Trace.printCallStack() -> String
// Returns the current call stack as a formatted string
static void trace_printCallStack(WrenVM* vm)
{
	ObjFiber* fiber = vm->fiber;

	if (fiber->numFrames == 0)
	{
		wrenSetSlotString(vm, 0, "Call stack is empty");
		return;
	}

	// Build the call stack string
	char buffer[4096] = "Call stack:\n";
	size_t offset = strlen(buffer);

	// Walk through all call frames from top to bottom
	for (int i = fiber->numFrames - 1; i >= 0; i--)
	{
		CallFrame* frame = &fiber->frames[i];
		ObjFn* fn = frame->closure->fn;

		// Calculate line number from instruction pointer
		int line = fn->debug->sourceLines.data[
			(int)(frame->ip - fn->code.data - 1)];

		// Get function name
		const char* fnName = fn->debug->name ? fn->debug->name : "<script>";

		// Get module name
		const char* moduleName = fn->module->name ?
			fn->module->name->value : "<unknown>";

		// Append to buffer
		int written = snprintf(buffer + offset, sizeof(buffer) - offset,
			"  [%d] %s in %s (line %d)\n",
			fiber->numFrames - 1 - i, fnName, moduleName, line);

		if (written < 0 || offset + written >= sizeof(buffer))
		{
			// Buffer overflow protection
			break;
		}

		offset += written;
	}

	wrenSetSlotString(vm, 0, buffer);
}

// Trace.getCallStack() -> List
// Returns the call stack as a list of strings
static void trace_getCallStack(WrenVM* vm)
{
	ObjFiber* fiber = vm->fiber;

	// Create a list to hold stack frames
	wrenSetSlotNewList(vm, 0);

	if (fiber->numFrames == 0)
	{
		return;
	}

	// Walk through all call frames
	for (int i = fiber->numFrames - 1; i >= 0; i--)
	{
		CallFrame* frame = &fiber->frames[i];
		ObjFn* fn = frame->closure->fn;

		// Calculate line number
		int line = fn->debug->sourceLines.data[
			(int)(frame->ip - fn->code.data - 1)];

		// Get function name
		const char* fnName = fn->debug->name ? fn->debug->name : "<script>";

		// Get module name
		const char* moduleName = fn->module->name ?
			fn->module->name->value : "<unknown>";

		// Format as string
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "%s in %s (line %d)",
			fnName, moduleName, line);

		// Add to list
		wrenSetSlotString(vm, 1, buffer);
		wrenInsertInList(vm, 0, -1, 1);
	}
}

// Trace.getStackDepth() -> Num
// Returns the current stack depth
static void trace_getStackDepth(WrenVM* vm)
{
	wrenSetSlotDouble(vm, 0, vm->fiber->numFrames);
}

// Bind foreign methods
WrenForeignMethodFn wrenTraceBindForeignMethod(WrenVM* vm,
	const char* className,
	bool isStatic,
	const char* signature)
{
	if (strcmp(className, "Trace") == 0)
	{
		if (isStatic)
		{
			if (strcmp(signature, "printCallStack()") == 0) return trace_printCallStack;
			if (strcmp(signature, "getCallStack()") == 0) return trace_getCallStack;
			if (strcmp(signature, "getStackDepth()") == 0) return trace_getStackDepth;
		}
	}

	return NULL;
}