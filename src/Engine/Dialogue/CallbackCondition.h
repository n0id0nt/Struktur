// WrenCondition.h
// Condition implementation that calls Wren callbacks
// Part of the Struktur dialogue system

#pragma once

#include "Condition.h"
#include "DialogueValue.h"
#include <map>
#include <string>
#include <functional>

namespace Struktur::Dialogue
{
	// Command that executes by calling a Wren callback
	class CallbackCondition : public Condition
	{
	private:
        using Callback = std::function<bool(const std::map<std::string, DialogueValue>& params)>;
		using DisposeCallback = std::function<void(GameContext& context)>;

	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		CallbackCondition(Callback callback, DisposeCallback disposeCallback);
		~CallbackCondition() override = default;

		void Dispose(GameContext& context) override;

		// Execute by calling Wren callback with parameters
		bool Evaluate(GameContext& context, const std::map<std::string, DialogueValue>& params) override;

	private:
		Callback m_callback;
		DisposeCallback m_disposeCallback;
		
	};
}