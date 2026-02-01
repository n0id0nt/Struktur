// WrenCommand.h
// Command implementation that calls Wren callbacks
// Part of the Struktur dialogue system

#pragma once

#include "Command.h"
#include "DialogueValue.h"
#include <map>
#include <string>

namespace Struktur::Dialogue
{
	// Command that executes by calling a Wren callback
	class CallbackCommand : public Command
	{
	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		CallbackCommand(std::string key, const std::unordered_map<std::string, DialogueValue>& params);
		~CallbackCommand() override = default;

		// Execute by calling Wren callback with parameters
		void Execute(GameContext& context) const override;

	private:
		std::string m_key;
		std::unordered_map<std::string, DialogueValue> m_params;

	};
}