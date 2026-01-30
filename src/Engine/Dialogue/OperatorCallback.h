// WrenCondition.h
// Condition implementation that calls Wren callbacks
// Part of the Struktur dialogue system

#pragma once

#include "Operator.h"
#include "DialogueValue.h"
#include <map>
#include <string>

namespace Struktur::Dialogue
{
	// Command that executes by calling a Wren callback
	class OperatorCondition : public Operator
	{
	public:
		// Constructor
		// Operator is borrowed from DialogueRegistry, not owned
		OperatorCondition(std::string key, const DialogueValue& lhs, const DialogueValue& rhs);

		// Execute by calling Wren callback with parameters
		bool Evaluate(GameContext& context) const override;

	private:
		std::string m_key;
		DialogueValue m_lhs;
        DialogueValue m_rhs;

	};
}