#include "wren_reflect.h"
#include "wren_vm.h"
#include "wren_value.h"
#include "wren_common.h"

#include <string.h>
#include "wren_reflect.wren.inc"

// The Wren source code (can be auto-generated from .wren file)
const char* wrenReflectSource()
{
	return reflectModuleSource;
}

// Reflect.hasMethod(class, signature) -> Bool
static void reflect_hasMethod(WrenVM* vm)
{
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL)
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	if (wrenGetSlotType(vm, 2) != WREN_TYPE_STRING)
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	const char* signature = wrenGetSlotString(vm, 2);

	// Access the class from the stack
	Value inputValue = vm->apiStack[1];
	ObjClass* classObj = NULL;

	if (IS_CLASS(inputValue))
	{
		classObj = AS_CLASS(inputValue);
	}
	else if (IS_INSTANCE(inputValue))
	{
		// It's an instance, get its class
		ObjInstance* instance = AS_INSTANCE(inputValue);
		classObj = instance->obj.classObj;
	}
	else if (IS_OBJ(inputValue))
	{
		// For other object types, get the class from the object
		Obj* obj = AS_OBJ(inputValue);
		classObj = obj->classObj;
	}
	else
	{
		// Primitive type (number, bool, etc.)
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	if (classObj == NULL)
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	// Look up method in symbol table
	int symbol = wrenSymbolTableFind(&vm->methodNames, signature, strlen(signature));

	bool hasMethod = false;

	if (symbol != -1 && symbol < classObj->methods.count)
	{
		Method* method = &classObj->methods.data[symbol];
		hasMethod = (method->type != METHOD_NONE);
	}

	wrenSetSlotBool(vm, 0, hasMethod);
}

// Reflect.getMethods(class) -> List
static void reflect_getMethods(WrenVM* vm)
{
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL)
	{
		wrenSetSlotNewList(vm, 0);
		return;
	}

	// Access the class from the stack
	Value inputValue = vm->apiStack[1];
	ObjClass* classObj = NULL;

	if (IS_CLASS(inputValue))
	{
		classObj = AS_CLASS(inputValue);
	}
	else if (IS_INSTANCE(inputValue))
	{
		// It's an instance, get its class
		ObjInstance* instance = AS_INSTANCE(inputValue);
		classObj = instance->obj.classObj;
	}
	else if (IS_OBJ(inputValue))
	{
		// For other object types, get the class from the object
		Obj* obj = AS_OBJ(inputValue);
		classObj = obj->classObj;
	}
	else
	{
		// Primitive type (number, bool, etc.)
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	if (classObj == NULL)
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	// Create list for methods
	wrenSetSlotNewList(vm, 0);

	for (int i = 0; i < classObj->methods.count; i++)
	{
		Method* method = &classObj->methods.data[i];

		if (method->type == METHOD_NONE) continue;

		// Get method name from symbol table
		if (i < vm->methodNames.count)
		{
			// methodNames.data might be ObjString** or Value* depending on version
			ObjString* nameString = vm->methodNames.data[i];

			if (nameString != NULL)
			{
				wrenSetSlotString(vm, 1, nameString->value);
				wrenInsertInList(vm, 0, -1, 1);
			}
		}
	}
}

// Reflect.getMethod(class, signature) -> String or null
static void reflect_getMethod(WrenVM* vm)
{
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	if (wrenGetSlotType(vm, 2) != WREN_TYPE_STRING)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	const char* signature = wrenGetSlotString(vm, 2);

	// Access the class from the stack
	Value inputValue = vm->apiStack[1];
	ObjClass* classObj = NULL;

	if (IS_CLASS(inputValue))
	{
		classObj = AS_CLASS(inputValue);
	}
	else if (IS_INSTANCE(inputValue))
	{
		// It's an instance, get its class
		ObjInstance* instance = AS_INSTANCE(inputValue);
		classObj = instance->obj.classObj;
	}
	else if (IS_OBJ(inputValue))
	{
		// For other object types, get the class from the object
		Obj* obj = AS_OBJ(inputValue);
		classObj = obj->classObj;
	}
	else
	{
		// Primitive type (number, bool, etc.)
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	if (classObj == NULL)
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}

	int symbol = wrenSymbolTableFind(&vm->methodNames, signature, strlen(signature));

	if (symbol == -1 || symbol >= classObj->methods.count)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	Method* method = &classObj->methods.data[symbol];

	if (method->type == METHOD_NONE)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, signature);
}

// Bind foreign methods
WrenForeignMethodFn wrenReflectBindForeignMethod(WrenVM* vm,
	const char* className,
	bool isStatic,
	const char* signature)
{
	if (strcmp(className, "Reflect") == 0)
	{
		if (isStatic)
		{
			if (strcmp(signature, "hasMethod_(_,_)") == 0) return reflect_hasMethod;
			if (strcmp(signature, "getMethods_(_)") == 0) return reflect_getMethods;
			if (strcmp(signature, "getMethod_(_,_)") == 0) return reflect_getMethod;
		}
	}

	return NULL;
}