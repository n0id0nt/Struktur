#include "Commands.h"
#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
	// CallbackCommand
	CallbackCommand::CallbackCommand(ExecutorFunc executor, const std::string& description)
		: m_executor(std::move(executor))
		, m_description(description)
	{
		ASSERT_MSG(m_executor != nullptr, "CallbackCommand executor cannot be null");
	}

	void CallbackCommand::Execute()
	{
		m_executor();
	}

	std::string CallbackCommand::GetDescription() const
	{
		return m_description;
	}

	// SequenceCommand
	SequenceCommand::SequenceCommand(std::vector<std::unique_ptr<Command>> commands)
		: m_commands(std::move(commands))
	{
		ASSERT_MSG(!m_commands.empty(), "SequenceCommand requires at least one command");
	}

	void SequenceCommand::Execute()
	{
		for (auto& command : m_commands)
		{
			command->Execute();
		}
	}

	std::string SequenceCommand::GetDescription() const
	{
		std::string desc = "SEQUENCE(";
		for (size_t i = 0; i < m_commands.size(); ++i)
		{
			if (i > 0)
				desc += ", ";
			desc += m_commands[i]->GetDescription();
		}
		desc += ")";
		return desc;
	}
}
