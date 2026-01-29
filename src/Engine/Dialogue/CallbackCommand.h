// WrenCommand.h
// Command implementation that calls Wren callbacks
// Part of the Struktur dialogue system

#pragma once

#include "Command.h"
#include "DialogueValue.h"
#include <map>
#include <string>
#include <functional>

namespace Struktur::Dialogue
{
	// Command that executes by calling a Wren callback
	class CallbackCommand : public Command
	{
	private:
        using Callback = std::function<void(const std::map<std::string, DialogueValue>& params)>;
		using DisposeCallback = std::function<void(GameContext& context)>;

	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		CallbackCommand(Callback callback, DisposeCallback disposeCallback);
		~CallbackCommand() override = default;

		void Dispose(GameContext& context) override;

		// Execute by calling Wren callback with parameters
		void Execute(GameContext& context, const std::map<std::string, DialogueValue>& params) override;

	private:
		Callback m_callback;
		DisposeCallback m_disposeCallback;
	};
}