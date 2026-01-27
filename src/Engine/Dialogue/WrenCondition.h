// WrenCondition.h
// Condition implementation that calls Wren callbacks
// Part of the Struktur dialogue system

#pragma once

#include "Condition.h"
#include "DialogueValue.h"
#include <map>
#include <string>

struct WrenVM;
struct WrenHandle;

namespace Struktur::Dialogue
{
	// Condition that evaluates by calling a Wren callback
	class WrenCondition : public Condition
	{
	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		WrenCondition(WrenVM* vm, WrenHandle* callback, const std::map<std::string, DialogueValue>& params);
		~WrenCondition() override = default;

		// Evaluate by calling Wren callback with parameters
		bool Evaluate(GameContext& context) override;

	private:
		WrenVM* m_vm;
		WrenHandle* m_callback;  // Borrowed pointer, not owned
		std::map<std::string, DialogueValue> m_parameters;
	};
}