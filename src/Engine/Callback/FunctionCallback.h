#pragma once

#include <functional>
#include <variant>
#include <vector>

#include "Callback.h"
#include "Variant.h"
#include "wren.hpp"

namespace Struktur::Callback
{
// C++ function/lambda callback implementation
class FunctionCallback : public ICallback
{
public:
	// Construct from function that takes variant vector
	FunctionCallback(std::function<Variant(const std::vector<Variant>&)> func);

	// Template constructor for any callable
	template <typename F>
	FunctionCallback(F&& func)
	    : m_function([f = std::forward<F>(func)](const std::vector<Variant>& args) -> Variant { return f(args); })
	{
	}

	~FunctionCallback() override = default;

	void Dispose(GameContext& context) override {}

	// ICallback interface
	Variant Invoke(GameContext& context, const std::vector<Variant>& args) override;
	bool IsValid() const override;

private:
	std::function<Variant(const std::vector<Variant>&)> m_function;
};
}  // namespace Struktur::Callback
