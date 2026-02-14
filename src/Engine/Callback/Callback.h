#pragma once

#include <memory>
#include "Variant.h"

#include "Debug/Assertions.h"

#include "CallbackHelperFunctions.h"

namespace Struktur
{
	struct GameContext;
}

namespace Struktur::Callback
{
	// Base interface for all callback implementations
	class ICallback
	{
	public:
		virtual ~ICallback() = default;
		virtual void Dispose(GameContext& context) = 0;
		virtual Variant Invoke(GameContext& context, const std::vector<Variant>& args) = 0;
		virtual bool IsValid() const = 0;
	};

	// Type-safe callback wrapper with enforced signature
	// Usage: TypedCallback<int(float, std::string)> callback;
	template<typename Signature>
	class TypedCallback;

	// Specialization for function signatures
	template<typename R, typename... Args>
	class TypedCallback<R(Args...)>
	{
	public:
		TypedCallback() = default;

		// Construct from ICallback pointer (takes ownership)
		explicit TypedCallback(std::unique_ptr<ICallback> callback)
		{
			m_callback = std::move(callback);
		}

		// Type-safe invoke - takes typed arguments, returns typed result
		R Invoke(GameContext& context, Args... args)
		{
			if (!m_callback)
			{
				DEBUG_WARNING("Attempted to invoke null TypedCallback");
				if constexpr (!std::is_void_v<R>)
				{
					return R{};
				}
				return;
			}

			// Convert arguments to variants
			std::vector<Variant> variants = HelperFunctions::ToVariants(std::forward<Args>(args)...);

			// Invoke the underlying callback
			Variant result = m_callback->Invoke(context, variants);

			// Extract typed result
			if constexpr (std::is_void_v<R>)
			{
				return;
			}
			else
			{
				try
				{
					return std::get<R>(result);
				}
				catch (const std::bad_variant_access&)
				{
					DEBUG_ERROR("TypedCallback return type mismatch");
					return R{};
				}
			}
		}

		void Dispose(GameContext& context)
		{
			m_callback->Dispose(context);
		}

		//// Operator() for natural calling syntax
		//R operator()(Args... args)
		//{
		//    return Invoke(std::forward<Args>(args)...);
		//}

		// Check validity
		bool IsValid() const
		{
			return m_callback && m_callback->IsValid();
		}

		explicit operator bool() const
		{
			return IsValid();
		}

		// Access underlying Callback
		ICallback* GetCallback() const
		{
			return m_callback.get();
		}

		// Release ownership of the underlying Callback
		std::unique_ptr<ICallback> Release()
		{
			return std::move(m_callback);
		}

	private:
		std::unique_ptr<ICallback> m_callback;

	};
}
