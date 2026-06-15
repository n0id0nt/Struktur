#include "WrenCallback.h"

#include "Debug/Assertions.h"
#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/GameContext.h"

namespace Struktur::Callback
{
// ============================================================================
// CALLBACK WREN IMPLEMENTATION
// ============================================================================

WrenCallback::WrenCallback(WrenHandle* handle)
    : m_callbackHandle(handle),
      m_disposed(false)
{
	ASSERT_MSG(handle != nullptr, "WrenHandle cannot be null");
}

WrenCallback::~WrenCallback()
{
	ASSERT_MSG(m_disposed, "WrenCallback has not been disposed");
}

void WrenCallback::Dispose(GameContext& context)
{
	WrenVM* vm = context.GetWrenScriptEngine().GetVM();
	if (m_callbackHandle)
	{
		wrenReleaseHandle(vm, m_callbackHandle);
	}

	m_disposed = true;
}

Variant WrenCallback::Invoke(GameContext& context, const std::vector<Variant>& args)
{
	WrenVM* vm = context.GetWrenScriptEngine().GetVM();
	if (!vm || !m_callbackHandle)
	{
		DEBUG_WARNING("Attempted to invoke invalid Wren callback");
		return Variant(nullptr);
	}

	// Ensure enough slots: 1 for receiver + args
	wrenEnsureSlots(vm, static_cast<int>(args.size() + 1));

	// Slot 0 is the callback (receiver)
	wrenSetSlotHandle(vm, 0, m_callbackHandle);

	// Slots 1..N are the arguments
	for (size_t i = 0; i < args.size(); ++i)
	{
		HelperFunctions::VariantToWrenSlot(vm, static_cast<int>(i + 1), args[i]);
	}

	std::string callHandleString = HelperFunctions::MakeWrenCallSignature(args.size());
	WrenHandle* method           = wrenMakeCallHandle(vm, callHandleString.c_str());

	// Call the Wren function
	WrenInterpretResult result = wrenCall(vm, method);

	wrenReleaseHandle(vm, method);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("Wren callback invocation failed");
		return Variant(nullptr);
	}

	// Extract return value from slot 0
	return HelperFunctions::WrenSlotToVariant(vm, 0);
}

bool WrenCallback::IsValid() const
{
	return m_callbackHandle != nullptr;
}
}  // namespace Struktur::Callback
