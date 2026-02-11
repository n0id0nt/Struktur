#pragma once

#include "Condition.h"
#include "DialogueValue.h"
#include <unordered_map>
#include <string>

namespace Struktur::Callback
{
	class ICallback;
}

namespace Struktur::Dialogue
{
	// Command that executes by calling a Wren callback
	class CallbackCondition : public Condition
	{
	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		CallbackCondition(std::string key, const std::unordered_map<std::string, DialogueValue>& params);
		~CallbackCondition() = default;

		// Execute by calling Wren callback with parameters
		bool Evaluate(GameContext& context) const override;
		Callback::ICallback* GetCallback(GameContext &context) const;

	private:
		std::string m_key;
		std::unordered_map<std::string, DialogueValue> m_params;

	};
}