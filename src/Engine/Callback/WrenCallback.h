#pragma once

#include <unordered_map>

#include "Callback.h"
#include "Variant.h"
#include "wren.hpp"

namespace Struktur::Callback
{
// Wren closure/function callback implementation
class WrenCallback : public ICallback
{
   public:
	// Construct from Wren VM and closure handle
	WrenCallback(WrenHandle* handle);
	~WrenCallback() override;

	void Dispose(GameContext& context);

	// Delete copy, allow move
	WrenCallback(const WrenCallback&)                = delete;
	WrenCallback& operator=(const WrenCallback&)     = delete;
	WrenCallback(WrenCallback&&) noexcept            = default;
	WrenCallback& operator=(WrenCallback&&) noexcept = default;

	// ICallback interface
	Variant Invoke(GameContext& context, const std::vector<Variant>& args) override;
	bool IsValid() const override;

	// Get the underlying Wren handle for passing back to Wren
	WrenHandle* GetHandle() const
	{
		return m_callbackHandle;
	}

   private:
	WrenHandle* m_callbackHandle;
	bool m_disposed;
};
}  // namespace Struktur::Callback
