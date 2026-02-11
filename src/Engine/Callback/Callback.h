#pragma once

#include "Variant.h"

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
}
