#include "DialogueNode.h"

namespace Struktur::Dialogue
{
	// DialogueChoice
	DialogueChoice::DialogueChoice(const std::string& text, const std::string& targetNode)
		: m_text(text)
		, m_targetNode(targetNode)
		, m_conditions()
	{
	}

	DialogueChoice::DialogueChoice(DialogueChoice&& other) noexcept
		: m_text(std::move(other.m_text))
		, m_targetNode(std::move(other.m_targetNode))
		, m_conditions(std::move(other.m_conditions))
	{
	}

	DialogueChoice& DialogueChoice::operator=(DialogueChoice&& other) noexcept
	{
		if (this != &other)
		{
			m_text = std::move(other.m_text);
			m_targetNode = std::move(other.m_targetNode);
			m_conditions = std::move(other.m_conditions);
		}
		return *this;
	}

	void DialogueChoice::AddCondition(std::unique_ptr<Condition> condition)
	{
		m_conditions.push_back(std::move(condition));
	}

	// DialogueNode
	DialogueNode::DialogueNode(const std::string& id)
		: m_id(id)
		, m_speaker()
		, m_text()
		, m_nextNode()
		, m_conditions()
		, m_commands()
		, m_choices()
	{
	}

	void DialogueNode::SetSpeaker(const std::string& speaker)
	{
		m_speaker = speaker;
	}

	void DialogueNode::SetText(const std::string& text)
	{
		m_text = text;
	}

	void DialogueNode::SetNextNode(const std::string& nodeId)
	{
		m_nextNode = nodeId;
	}

	void DialogueNode::AddCondition(std::unique_ptr<Condition> condition)
	{
		m_conditions.push_back(std::move(condition));
	}

	void DialogueNode::AddCommand(std::unique_ptr<Command> command)
	{
		m_commands.push_back(std::move(command));
	}

	void DialogueNode::AddChoice(DialogueChoice choice)
	{
		m_choices.push_back(std::move(choice));
	}
}
