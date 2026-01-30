#include "DialogueNode.h"

namespace Struktur::Dialogue
{
	DialogueNode::DialogueNode(const std::string& id)
		: m_id(id)
		, m_speaker(std::nullopt)
		, m_text(std::nullopt)
		, m_commands()
		, m_choices()
		, m_next(std::nullopt)
		, m_targets()
	{
	}

	void DialogueNode::AddCommand(const Command& command)
	{
		m_commands.push_back(command);
	}

	void DialogueNode::AddChoice(const Choice& choice)
	{
		m_choices.push_back(choice);
	}

	void DialogueNode::AddTarget(ConditionalTarget&& target)
	{
		m_targets.push_back(std::move(target));
	}
}
