// WrenCommand.h
// Command implementation that calls Wren callbacks
// Part of the Struktur dialogue system

#pragma once

#include "Command.h"
#include "DialogueValue.h"
#include <map>
#include <string>

struct WrenVM;
struct WrenHandle;

namespace Struktur::Dialogue
{
	// Command that executes by calling a Wren callback
	class WrenCommand : public Command
	{
	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		WrenCommand(WrenVM* vm, WrenHandle* callback, const std::map<std::string, DialogueValue>& params);
		~WrenCommand() override = default;

		// Execute by calling Wren callback with parameters
		void Execute(GameContext& context) override;

	private:
		WrenVM* m_vm;
		WrenHandle* m_callback;  // Borrowed pointer, not owned
		std::map<std::string, DialogueValue> m_parameters;
	};
}