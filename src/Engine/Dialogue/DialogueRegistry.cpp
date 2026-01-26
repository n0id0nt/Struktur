#include "DialogueRegistry.h"

namespace Struktur::Dialogue
{
	void DialogueRegistry::RegisterConditionType(const std::string& typeName, ConditionFactory factory)
	{
		m_conditionFactories[typeName] = std::move(factory);
	}

	std::unique_ptr<Condition> DialogueRegistry::CreateCondition(
		const std::string& typeName,
		const std::unordered_map<std::string, std::string>& params
	) const
	{
		auto it = m_conditionFactories.find(typeName);
		if (it == m_conditionFactories.end())
			return nullptr;
		
		return it->second(params);
	}

	void DialogueRegistry::RegisterCommandType(const std::string& typeName, CommandFactory factory)
	{
		m_commandFactories[typeName] = std::move(factory);
	}

	std::unique_ptr<Command> DialogueRegistry::CreateCommand(
		const std::string& typeName,
		const std::unordered_map<std::string, std::string>& params
	) const
	{
		auto it = m_commandFactories.find(typeName);
		if (it == m_commandFactories.end())
			return nullptr;
		
		return it->second(params);
	}

	std::vector<std::string> DialogueRegistry::GetRegisteredConditionTypes() const
	{
		std::vector<std::string> types;
		types.reserve(m_conditionFactories.size());
		
		for (const auto& [type, _] : m_conditionFactories)
			types.push_back(type);
		
		return types;
	}

	std::vector<std::string> DialogueRegistry::GetRegisteredCommandTypes() const
	{
		std::vector<std::string> types;
		types.reserve(m_commandFactories.size());
		
		for (const auto& [type, _] : m_commandFactories)
			types.push_back(type);
		
		return types;
	}

	bool DialogueRegistry::HasConditionType(const std::string& typeName) const
	{
		return m_conditionFactories.find(typeName) != m_conditionFactories.end();
	}

	bool DialogueRegistry::HasCommandType(const std::string& typeName) const
	{
		return m_commandFactories.find(typeName) != m_commandFactories.end();
	}
}
