// DialogueRegistry.h
// Manages registration of condition, command, and operator callbacks
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "Condition.h"
#include "Command.h"
#include "DialogueValue.h"

#include "Engine/Callback/Callback.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
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
		void RegisterConditionType(GameContext& context, const std::string& type, std::unique_ptr<Callback::ICallback> callback);

		// Register command type with Wren callback
		void RegisterCommandType(GameContext& context, const std::string& type, std::unique_ptr<Callback::ICallback> callback);

		// Register variable type with Wren callback
		void RegisterVariableType(GameContext& context, const std::string& type, std::unique_ptr<Callback::ICallback> callback);

		// Check if type is registered
		bool HasConditionType(const std::string& type) const;
		bool HasCommandType(const std::string& type) const;
		bool HasVariableType(const std::string& type) const;

		// Check if type is registered
		Callback::ICallback* GetCondition(const std::string& type) const;
		Callback::ICallback* GetCommand(const std::string& type) const;
		Callback::ICallback* GetVariable(const std::string& type) const;

	private:
		std::unordered_map<std::string, std::unique_ptr<Callback::ICallback>> m_conditionCallbacks;
		std::unordered_map<std::string, std::unique_ptr<Callback::ICallback>> m_commandCallbacks;
		std::unordered_map<std::string, std::unique_ptr<Callback::ICallback>> m_variableCallbacks;
	};
}
