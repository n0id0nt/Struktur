#include "FunctionCallback.h"

#include "Debug/Assertions.h"

namespace Struktur::Callback
{
    // ============================================================================
	// CALLBACK FUNCTION IMPLEMENTATION
	// ============================================================================
	
	FunctionCallback::FunctionCallback(std::function<Variant(const std::vector<Variant>&)> func)
		: m_function(std::move(func))
	{
	}
	
	Variant FunctionCallback::Invoke(GameContext& context, const std::vector<Variant>& args)
	{
		if (!m_function)
		{
			DEBUG_WARNING("Attempted to invoke null C++ callback");
			return Variant(nullptr);
		}
		
		return m_function(args);
	}
	
	bool FunctionCallback::IsValid() const
	{
		return (bool)m_function;
	}
}
