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
	struct Choice;
	struct ConditionalTarget;

	//TODO find a better file to put this
	using CommandList = std::vector<std::unique_ptr<Command>>;
	using ConditionList = std::vector<std::unique_ptr<Condition>>;
	using ChoiceList = std::vector<std::unique_ptr<Choice>>;
	using ConditionalTargetList = std::vector<std::unique_ptr<ConditionalTarget>>;

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
		ConditionList conditions;
		std::string targetNode;

		ConditionalTarget() : conditions(), targetNode("") {}
	};

	// A single node in the dialogue graph
	class DialogueNode
	{
	public:
		DialogueNode(const std::string& id);
		~DialogueNode() = default;

		// can't copy due to having unique pointers.
		DialogueNode(const DialogueNode&) = delete;
		DialogueNode& operator=(const DialogueNode&) = delete;

		DialogueNode(DialogueNode&&) = default;
		DialogueNode& operator=(DialogueNode&&) = default;

		// Getters
		const std::string& GetId() const { return m_id; }
		const std::optional<std::string>& GetSpeaker() const { return m_speaker; }
		const std::optional<std::string>& GetText() const { return m_text; }
		const CommandList& GetCommands() const { return m_commands; }
		const ChoiceList& GetChoices() const { return m_choices; }
		const std::optional<std::string>& GetNext() const { return m_next; }
		const ConditionalTargetList& GetTargets() const { return m_targets; }

		// Setters
		void SetSpeaker(const std::string& speaker) { m_speaker = speaker; }
		void SetText(const std::string& text) { m_text = text; }
		void SetNext(const std::string& next) { m_next = next; }

		// Add methods
		void AddCommand(std::unique_ptr<Command> command);
		void AddChoice(std::unique_ptr<Choice> choice);
		void AddTarget(std::unique_ptr<ConditionalTarget> target);

		// Check node type
		bool HasChoices() const { return !m_choices.empty(); }
		bool HasNext() const { return m_next.has_value(); }
		bool HasTargets() const { return !m_targets.empty(); }

	private:
		std::string m_id;
		std::optional<std::string> m_speaker;
		std::optional<std::string> m_text;
		CommandList m_commands;
		ChoiceList m_choices;
		std::optional<std::string> m_next;
		ConditionalTargetList m_targets;
	};
}
