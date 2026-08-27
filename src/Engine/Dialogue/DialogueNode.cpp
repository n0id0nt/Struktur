#include "DialogueNode.h"

namespace Struktur::Dialogue
{
DialogueNode::DialogueNode(const std::string& id)
    : m_id(id),
      m_speaker(std::nullopt),
      m_text(std::nullopt),
      m_commands(),
      m_choices(),
      m_next(std::nullopt),
      m_targets()
{
}

void DialogueNode::AddCommand(std::unique_ptr<Command> command)
{
	m_commands.push_back(std::move(command));
}

void DialogueNode::AddChoice(std::unique_ptr<Choice> choice)
{
	m_choices.push_back(std::move(choice));
}

void DialogueNode::AddTarget(std::unique_ptr<ConditionalTarget> target)
{
	m_targets.push_back(std::move(target));
}

void DialogueNode::RemoveCommand(size_t index)
{
	if (index < m_commands.size())
	{
		m_commands.erase(m_commands.begin() + index);
	}
}

void DialogueNode::RemoveChoice(size_t index)
{
	if (index < m_choices.size())
	{
		m_choices.erase(m_choices.begin() + index);
	}
}

void DialogueNode::RemoveTarget(size_t index)
{
	if (index < m_targets.size())
	{
		m_targets.erase(m_targets.begin() + index);
	}
}

void DialogueNode::ClearNext()
{
	m_next.reset();
}

void DialogueNode::ClearChoices()
{
	m_choices.clear();
}

void DialogueNode::ClearTargets()
{
	m_targets.clear();
}
}  // namespace Struktur::Dialogue
