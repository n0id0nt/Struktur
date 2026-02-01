// DialogueRegistry.h
// Manages registration of condition, command, and operator callbacks
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

#include "Condition.h"
#include "Command.h"
#include "DialogueValue.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	using ConditionCallback = std::function<bool(const std::unordered_map<std::string, DialogueValue>& params)>;
	using CommandCallback = std::function<void(const std::unordered_map<std::string, DialogueValue>& params)>;
	using OperatorCallback = std::function<bool(const DialogueValue& lhs, const DialogueValue& rhs)>;
	using DisposeCallback = std::function<void(GameContext& context)>;

	template<typename CallBack>
	struct CallBackPair
	{
		CallBack callback;
		DisposeCallback disposeCallBack;
	};

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
		void RegisterConditionType(GameContext& context, const std::string& type, ConditionCallback&& callback, DisposeCallback&& DisposeCallback);

		// Register command type with Wren callback
		void RegisterCommandType(GameContext& context, const std::string& type, CommandCallback&& callback, DisposeCallback&& DisposeCallback);

		// Register operator with Wren callback
		void RegisterOperator(GameContext& context, const std::string& op, OperatorCallback&& callback, DisposeCallback&& DisposeCallback);

		// Evaluate operator
		bool EvaluateOperator(GameContext& context, const std::string& op, const DialogueValue& lhs, const DialogueValue& rhs);

		// Check if type/operator is registered
		bool HasConditionType(const std::string& type) const;
		bool HasCommandType(const std::string& type) const;
		bool HasOperator(const std::string& op) const;

		// Check if type/operator is registered
		ConditionCallback& GetCondition(const std::string& type);
		CommandCallback& GetCommand(const std::string& type);
		OperatorCallback& GetOperator(const std::string& op);

	private:
		std::unordered_map<std::string, CallBackPair<ConditionCallback>> m_conditionCallbacks;
		std::unordered_map<std::string, CallBackPair<CommandCallback>> m_commandCallbacks;
		std::unordered_map<std::string, CallBackPair<OperatorCallback>> m_operatorCallbacks;
	};
}
