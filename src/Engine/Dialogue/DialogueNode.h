// DialogueNode.h
// Dialogue node structure containing text, choices, conditions, and commands
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "Condition.h"
#include "Command.h"

namespace Struktur::Dialogue
{
	// Represents a player choice in dialogue
	struct Choice
	{
		std::string text;
		std::string targetNode;

		Choice() : text(""), targetNode("") {}
		Choice(const std::string& t, const std::string& target) : text(t), targetNode(target) {}
	};

	// Represents a conditional branch with conditions and target
	struct ConditionalTarget
	{
		std::vector<Condition> conditions;
		std::string targetNode;

		ConditionalTarget() : conditions(), targetNode("") {}
	};

	// A single node in the dialogue graph
	class DialogueNode
	{
	public:
		DialogueNode(const std::string& id);
		~DialogueNode() = default;

		// Delete copy, allow move
		DialogueNode(const DialogueNode&) = delete;
		DialogueNode& operator=(const DialogueNode&) = delete;
		DialogueNode(DialogueNode&&) noexcept = default;
		DialogueNode& operator=(DialogueNode&&) noexcept = default;

		// Getters
		const std::string& GetId() const { return m_id; }
		const std::optional<std::string>& GetSpeaker() const { return m_speaker; }
		const std::optional<std::string>& GetText() const { return m_text; }
		const std::vector<Command>& GetCommands() const { return m_commands; }
		const std::vector<Choice>& GetChoices() const { return m_choices; }
		const std::optional<std::string>& GetNext() const { return m_next; }
		const std::vector<ConditionalTarget>& GetTargets() const { return m_targets; }

		// Setters
		void SetSpeaker(const std::string& speaker) { m_speaker = speaker; }
		void SetText(const std::string& text) { m_text = text; }
		void SetNext(const std::string& next) { m_next = next; }

		// Add methods
		void AddCommand(const Command& command);
		void AddChoice(const Choice& choice);
		void AddTarget(ConditionalTarget&& target);

		// Check node type
		bool HasChoices() const { return !m_choices.empty(); }
		bool HasNext() const { return m_next.has_value(); }
		bool HasTargets() const { return !m_targets.empty(); }

	private:
		std::string m_id;
		std::optional<std::string> m_speaker;
		std::optional<std::string> m_text;
		std::vector<Command> m_commands;
		std::vector<Choice> m_choices;
		std::optional<std::string> m_next;
		std::vector<ConditionalTarget> m_targets;
	};
}
