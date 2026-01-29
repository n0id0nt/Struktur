// DialogueRegistry.h
// Manages registration of condition, command, and operator callbacks
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <map>
#include <memory>

#include "Condition.h"
#include "Command.h"
#include "DialogueValue.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Registry for dialogue conditions, commands, and operators
	// Stores Wren callback handles for runtime evaluation/execution
	class DialogueRegistry
	{
	public:
		explicit DialogueRegistry();
		~DialogueRegistry();

		void Clear(GameContext& context);

		// Delete copy and move
		DialogueRegistry(const DialogueRegistry&) = delete;
		DialogueRegistry& operator=(const DialogueRegistry&) = delete;
		DialogueRegistry(DialogueRegistry&&) = delete;
		DialogueRegistry& operator=(DialogueRegistry&&) = delete;

		// Register condition type with Wren callback
		void RegisterConditionType(GameContext& context, const std::string& type, WrenHandle* callback);

		// Register command type with Wren callback
		void RegisterCommandType(GameContext& context, const std::string& type, WrenHandle* callback);

		// Register operator with Wren callback
		void RegisterOperator(GameContext& context, const std::string& op, WrenHandle* callback);

		// Create condition from registered type
		std::unique_ptr<Condition> CreateCondition(GameContext& context, const std::string& type, const std::map<std::string, DialogueValue>& params);

		// Create command from registered type
		std::unique_ptr<Command> CreateCommand(GameContext& context, const std::string& type, const std::map<std::string, DialogueValue>& params);

		// Evaluate operator
		bool EvaluateOperator(GameContext& context, const std::string& op, const DialogueValue& lhs, const DialogueValue& rhs);

		// Check if type/operator is registered
		bool HasConditionType(const std::string& type) const;
		bool HasCommandType(const std::string& type) const;
		bool HasOperator(const std::string& op) const;

	private:
		std::map<std::string, WrenHandle*> m_conditionCallbacks;
		std::map<std::string, WrenHandle*> m_commandCallbacks;
		std::map<std::string, WrenHandle*> m_operatorCallbacks;
	};
}
