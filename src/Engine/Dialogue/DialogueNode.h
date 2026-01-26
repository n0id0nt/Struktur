#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Conditions.h"
#include "Commands.h"

namespace Struktur::Dialogue
{
	class DialogueChoice
	{
	public:
		DialogueChoice(const std::string& text, const std::string& targetNode);
		~DialogueChoice() = default;

		// Copy constructor and assignment
		DialogueChoice(const DialogueChoice& other) = delete;
		DialogueChoice& operator=(const DialogueChoice& other) = delete;

		// Move constructor and assignment
		DialogueChoice(DialogueChoice&& other) noexcept;
		DialogueChoice& operator=(DialogueChoice&& other) noexcept;

		void AddCondition(std::unique_ptr<Condition> condition);

		const std::string& GetText() const { return m_text; }
		const std::string& GetTargetNode() const { return m_targetNode; }
		const std::vector<std::unique_ptr<Condition>>& GetConditions() const { return m_conditions; }

	private:
		std::string m_text;
		std::string m_targetNode;
		std::vector<std::unique_ptr<Condition>> m_conditions;
	};

	class DialogueNode
	{
	public:
		DialogueNode(const std::string& id);
		~DialogueNode() = default;

		// Delete copy constructor and assignment
		DialogueNode(const DialogueNode&) = delete;
		DialogueNode& operator=(const DialogueNode&) = delete;

		// Default move constructor and assignment
		DialogueNode(DialogueNode&&) noexcept = default;
		DialogueNode& operator=(DialogueNode&&) noexcept = default;

		void SetSpeaker(const std::string& speaker);
		void SetText(const std::string& text);
		void SetNextNode(const std::string& nodeId);
		void AddCondition(std::unique_ptr<Condition> condition);
		void AddCommand(std::unique_ptr<Command> command);
		void AddChoice(DialogueChoice choice);

		const std::string& GetId() const { return m_id; }
		const std::string& GetSpeaker() const { return m_speaker; }
		const std::string& GetText() const { return m_text; }
		const std::string& GetNextNode() const { return m_nextNode; }
		const std::vector<std::unique_ptr<Condition>>& GetConditions() const { return m_conditions; }
		const std::vector<std::unique_ptr<Command>>& GetCommands() const { return m_commands; }
		const std::vector<DialogueChoice>& GetChoices() const { return m_choices; }

	private:
		std::string m_id;
		std::string m_speaker;
		std::string m_text;
		std::string m_nextNode;
		std::vector<std::unique_ptr<Condition>> m_conditions;
		std::vector<std::unique_ptr<Command>> m_commands;
		std::vector<DialogueChoice> m_choices;
	};
}
