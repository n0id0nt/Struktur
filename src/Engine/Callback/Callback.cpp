// Callback.cpp
// Implementation of unified callback system

#include "Callback.h"

#include "Debug/Assertions.h"

namespace Struktur::Callback
{
	// ============================================================================
	// CALLBACK FUNCTION IMPLEMENTATION
	// ============================================================================
	
	CallbackFunction::CallbackFunction(std::function<Variant(const std::vector<Variant>&)> func)
		: m_function(std::move(func))
	{
	}
	
	Variant CallbackFunction::Invoke(const std::vector<Variant>& args)
	{
		if (!m_function)
		{
			DEBUG_WARNING("Attempted to invoke null C++ callback");
			return Variant(nullptr);
		}
		
		return m_function(args);
	}
	
	bool CallbackFunction::IsValid() const
	{
		return m_function != nullptr;
	}
	
	// ============================================================================
	// CALLBACK WREN IMPLEMENTATION
	// ============================================================================
	
	CallbackWren::CallbackWren(WrenVM* vm, WrenHandle* handle)
		: m_vm(vm)
		, m_callbackHandle(handle)
		, m_callMethodHandles()
	{
		ASSERT_MSG(vm != nullptr, "WrenVM cannot be null");
		ASSERT_MSG(handle != nullptr, "WrenHandle cannot be null");
	}
	
	CallbackWren::~CallbackWren()
	{
		if (m_callbackHandle)
		{
			wrenReleaseHandle(m_vm, m_callbackHandle);
		}
		
		// Release all cached method handles
		for (auto& [argCount, handle] : m_callMethodHandles)
		{
			wrenReleaseHandle(m_vm, handle);
		}
	}
	
	WrenHandle* CallbackWren::GetCallMethodHandle(size_t argCount) const
	{
		auto it = m_callMethodHandles.find(argCount);
		if (it != m_callMethodHandles.end())
		{
			return it->second;
		}
		
		// Create new call signature
		std::string signature = MakeWrenCallSignature(argCount);
		WrenHandle* handle = wrenMakeCallHandle(m_vm, signature.c_str());
		m_callMethodHandles[argCount] = handle;
		
		return handle;
	}
	
	Variant CallbackWren::Invoke(const std::vector<Variant>& args)
	{
		if (!m_vm || !m_callbackHandle)
		{
			DEBUG_WARNING("Attempted to invoke invalid Wren callback");
			return Variant(nullptr);
		}
		
		// Ensure enough slots: 1 for receiver + args
		wrenEnsureSlots(m_vm, static_cast<int>(args.size() + 1));
		
		// Slot 0 is the callback (receiver)
		wrenSetSlotHandle(m_vm, 0, m_callbackHandle);
		
		// Slots 1..N are the arguments
		for (size_t i = 0; i < args.size(); ++i)
		{
			VariantToWrenSlot(m_vm, static_cast<int>(i + 1), args[i]);
		}
		
		// Get the call method handle for this argument count
		WrenHandle* methodHandle = GetCallMethodHandle(args.size());
		if (!methodHandle)
		{
			DEBUG_ERROR("Failed to create Wren call method handle for %zu arguments", args.size());
			return Variant(nullptr);
		}
		
		// Call the Wren function
		WrenInterpretResult result = wrenCall(m_vm, methodHandle);
		
		if (result != WREN_RESULT_SUCCESS)
		{
			DEBUG_ERROR("Wren callback invocation failed");
			return Variant(nullptr);
		}
		
		// Extract return value from slot 0
		return WrenSlotToVariant(m_vm, 0);
	}
	
	bool CallbackWren::IsValid() const
	{
		return m_vm != nullptr && m_callbackHandle != nullptr;
	}
	
	// ============================================================================
	// CALLBACK WRAPPER IMPLEMENTATION
	// ============================================================================
	
	Callback Callback::FromWren(WrenVM* vm, int slot)
	{
		Callback cb;
		WrenHandle* handle = wrenGetSlotHandle(vm, slot);
		cb.m_impl = std::make_unique<CallbackWren>(vm, handle);
		return cb;
	}
	
	Variant Callback::Invoke(const std::vector<Variant>& args)
	{
		if (!m_impl)
		{
			DEBUG_WARNING("Attempted to invoke null callback");
			return Variant(nullptr);
		}
		
		return m_impl->Invoke(args);
	}
	
	bool Callback::IsValid() const
	{
		return m_impl && m_impl->IsValid();
	}
	
	bool Callback::IsWrenCallback() const
	{
		return dynamic_cast<CallbackWren*>(m_impl.get()) != nullptr;
	}
	
	bool Callback::IsCppCallback() const
	{
		return dynamic_cast<CallbackFunction*>(m_impl.get()) != nullptr;
	}
	
	WrenHandle* Callback::GetWrenHandle() const
	{
		if (auto wrenImpl = dynamic_cast<CallbackWren*>(m_impl.get()))
		{
			return wrenImpl->GetHandle();
		}
		return nullptr;
	}
	
	WrenVM* Callback::GetWrenVM() const
	{
		if (auto wrenImpl = dynamic_cast<CallbackWren*>(m_impl.get()))
		{
			return wrenImpl->GetVM();
		}
		return nullptr;
	}
	
} // namespace Struktur