// Callback.h
// Unified callback system for C++ and Wren integration
// Supports lambdas, function pointers, and Wren closures with automatic type marshalling
// Part of the Struktur scripting system

#pragma once

#include <variant>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include "wren.hpp"


namespace Struktur::Callback
{
	// ============================================================================
	// VARIANT TYPE DEFINITION
	// ============================================================================
	
	// Variant type for cross-language argument passing between C++ and Wren
	using Variant = std::variant<
		std::nullptr_t,
		bool,
		int,
		float,
		double,
		std::string
	>;
	
	// ============================================================================
	// HELPER FUNCTIONS
	// ============================================================================
	
	// Generate Wren call signature like "call(_,_,_)" based on argument count
	inline std::string MakeWrenCallSignature(size_t argCount)
	{
		if (argCount == 0)
		{
			return "call()";
		}
		
		std::string sig = "call(";
		for (size_t i = 0; i < argCount; ++i)
		{
			if (i > 0) sig += ",";
			sig += "_";
		}
		sig += ")";
		return sig;
	}
	
	// Push a variant value to a Wren slot
	inline void VariantToWrenSlot(WrenVM* vm, int slot, const Variant& value)
	{
		std::visit([vm, slot](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			
			if constexpr (std::is_same_v<T, std::nullptr_t>)
			{
				wrenSetSlotNull(vm, slot);
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				wrenSetSlotBool(vm, slot, arg);
			}
			else if constexpr (std::is_same_v<T, int>)
			{
				wrenSetSlotDouble(vm, slot, static_cast<double>(arg));
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				wrenSetSlotDouble(vm, slot, static_cast<double>(arg));
			}
			else if constexpr (std::is_same_v<T, double>)
			{
				wrenSetSlotDouble(vm, slot, arg);
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				wrenSetSlotString(vm, slot, arg.c_str());
			}
		}, value);
	}
	
	// Extract a variant value from a Wren slot
	inline Variant WrenSlotToVariant(WrenVM* vm, int slot)
	{
		WrenType type = wrenGetSlotType(vm, slot);
		
		switch (type)
		{
			case WREN_TYPE_BOOL:
				return Variant(wrenGetSlotBool(vm, slot));
				
			case WREN_TYPE_NUM:
				return Variant(wrenGetSlotDouble(vm, slot));
				
			case WREN_TYPE_STRING:
				return Variant(std::string(wrenGetSlotString(vm, slot)));
				
			case WREN_TYPE_NULL:
			default:
				return Variant(nullptr);
		}
	}
	
	// Convert variadic arguments to vector of variants
	template<typename... Args>
	std::vector<Variant> ToVariants(Args&&... args)
	{
		std::vector<Variant> result;
		result.reserve(sizeof...(Args));
		(result.push_back(Variant(std::forward<Args>(args))), ...);
		return result;
	}
	
	// ============================================================================
	// CALLBACK INTERFACE
	// ============================================================================
	
	// Base interface for all callback implementations
	class ICallback
	{
	public:
		virtual ~ICallback() = default;
		
		// Invoke callback with variant arguments
		// @param args - Vector of variant arguments to pass to the callback
		// @return Variant containing the return value (nullptr for void)
		virtual Variant Invoke(const std::vector<Variant>& args) = 0;
		
		// Check if callback is valid and can be invoked
		virtual bool IsValid() const = 0;
	};
	
	// ============================================================================
	// C++ FUNCTION CALLBACK
	// ============================================================================
	
	// C++ function/lambda callback implementation
	class CallbackFunction : public ICallback
	{
	public:
		// Construct from function that takes variant vector
		explicit CallbackFunction(std::function<Variant(const std::vector<Variant>&)> func);
		
		// Template constructor for any callable
		template<typename F>
		explicit CallbackFunction(F&& func)
			: m_function([f = std::forward<F>(func)](const std::vector<Variant>& args) -> Variant
			{
				return f(args);
			})
		{
		}
		
		~CallbackFunction() override = default;
		
		// ICallback interface
		Variant Invoke(const std::vector<Variant>& args) override;
		bool IsValid() const override;
		
	private:
		std::function<Variant(const std::vector<Variant>&)> m_function;
	};
	
	// ============================================================================
	// WREN CALLBACK
	// ============================================================================
	
	// Wren closure/function callback implementation
	class CallbackWren : public ICallback
	{
	public:
		// Construct from Wren VM and closure handle
		// @param vm - Wren VM that owns the closure
		// @param handle - WrenHandle to the closure (ownership transferred)
		CallbackWren(WrenVM* vm, WrenHandle* handle);
		~CallbackWren() override;
		
		// Delete copy, allow move
		CallbackWren(const CallbackWren&) = delete;
		CallbackWren& operator=(const CallbackWren&) = delete;
		CallbackWren(CallbackWren&&) noexcept = default;
		CallbackWren& operator=(CallbackWren&&) noexcept = default;
		
		// ICallback interface
		Variant Invoke(const std::vector<Variant>& args) override;
		bool IsValid() const override;
		
		// Get the underlying Wren handle for passing back to Wren
		WrenHandle* GetHandle() const { return m_callbackHandle; }
		WrenVM* GetVM() const { return m_vm; }
		
	private:
		WrenVM* m_vm;
		WrenHandle* m_callbackHandle;
		
		// Cached call method handles for different argument counts
		// Maps argument count -> WrenHandle for "call(_,_,...)" method
		mutable std::unordered_map<size_t, WrenHandle*> m_callMethodHandles;
		
		// Get or create a call method handle for the given argument count
		WrenHandle* GetCallMethodHandle(size_t argCount) const;
	};
	
	// ============================================================================
	// CALLBACK WRAPPER
	// ============================================================================
	
	// User-facing callback wrapper that works with both C++ and Wren callbacks
	class Callback
	{
	public:
		// Default constructor creates null callback
		Callback() = default;
		
		// Create callback from C++ lambda/function
		// @param func - Any callable that accepts vector<Variant> and returns Variant
		template<typename F>
		static Callback FromCpp(F&& func)
		{
			Callback cb;
			cb.m_impl = std::make_unique<CallbackFunction>(std::forward<F>(func));
			return cb;
		}
		
		// Create callback from Wren closure
		// @param vm - Wren VM
		// @param slot - Slot containing the Wren closure
		static Callback FromWren(WrenVM* vm, int slot);
		
		// Invoke callback with variant vector
		Variant Invoke(const std::vector<Variant>& args);
		
		// Invoke callback with typed arguments (converted to variants)
		template<typename... Args>
		Variant operator()(Args&&... args)
		{
			std::vector<Variant> variants = ToVariants(std::forward<Args>(args)...);
			return Invoke(variants);
		}
		
		// Check if callback is valid
		bool IsValid() const;
		explicit operator bool() const { return IsValid(); }
		
		// Type checking
		bool IsWrenCallback() const;
		bool IsCppCallback() const;
		
		// Get Wren handle (returns nullptr if not a Wren callback)
		WrenHandle* GetWrenHandle() const;
		WrenVM* GetWrenVM() const;
		
	private:
		std::unique_ptr<ICallback> m_impl;
	};
	
} // namespace Struktur