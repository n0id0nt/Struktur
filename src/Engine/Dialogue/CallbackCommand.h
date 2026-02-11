#pragma once

#include "Command.h"
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
	class CallbackCommand : public Command
	{
	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		CallbackCommand(std::string key, const std::unordered_map<std::string, DialogueValue>& params);
		~CallbackCommand() = default;

		// Execute by calling Wren callback with parameters
		void Execute(GameContext& context) const override;
		Callback::ICallback* GetCallback(GameContext &context) const;

	private:
		std::string m_key;
		std::unordered_map<std::string, DialogueValue> m_params;

	};
}