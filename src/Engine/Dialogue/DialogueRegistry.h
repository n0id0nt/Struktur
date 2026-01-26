#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "Conditions.h"
#include "Commands.h"

namespace Struktur::Dialogue
{
	class DialogueRegistry
	{
	public:
		// Condition factory registration
		using ConditionFactory = std::function<std::unique_ptr<Condition>(
			const std::unordered_map<std::string, std::string>& params
		)>;

		void RegisterConditionType(const std::string& typeName, ConditionFactory factory);
		std::unique_ptr<Condition> CreateCondition(
			const std::string& typeName,
			const std::unordered_map<std::string, std::string>& params
		) const;

		// Command factory registration
		using CommandFactory = std::function<std::unique_ptr<Command>(
			const std::unordered_map<std::string, std::string>& params
		)>;

		void RegisterCommandType(const std::string& typeName, CommandFactory factory);
		std::unique_ptr<Command> CreateCommand(
			const std::string& typeName,
			const std::unordered_map<std::string, std::string>& params
		) const;

		// Query registered types
		std::vector<std::string> GetRegisteredConditionTypes() const;
		std::vector<std::string> GetRegisteredCommandTypes() const;
		bool HasConditionType(const std::string& typeName) const;
		bool HasCommandType(const std::string& typeName) const;

	private:
		std::unordered_map<std::string, ConditionFactory> m_conditionFactories;
		std::unordered_map<std::string, CommandFactory> m_commandFactories;
	};
}
