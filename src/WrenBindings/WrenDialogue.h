#pragma once

#include "Engine/Dialogue/DialogueResult.h"
#include "Engine/Dialogue/DialogueNode.h"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrapper for DialogueResult
struct WrenDialogueResult
{
	Struktur::Dialogue::DialogueResult dataResult;

	WrenDialogueResult(Struktur::Dialogue::DialogueResult& dataResult) : dataResult(dataResult) {}
};

// Wrapper for DialogueNode
struct WrenDialogueNode
{
	Struktur::Dialogue::DialogueNode* dataNode;

	WrenDialogueNode(Struktur::Dialogue::DialogueNode* dataNode) : dataNode(dataNode) {}
};

// Wrapper for ConditionalTarget
struct WrenConditionalTarget
{
	Struktur::Dialogue::ConditionalTarget* target;

	WrenConditionalTarget(Struktur::Dialogue::ConditionalTarget* target) : target(target) {}
};

// Wrapper for Dialogue Condition
struct WrenCondition
{
	Struktur::Dialogue::Condition* condition;

	WrenCondition(Struktur::Dialogue::Condition* condition) : condition(condition) {}
};

// Wrapper for Dialogue Command
struct WrenCommand
{
	Struktur::Dialogue::Command* command;

	WrenCommand(Struktur::Dialogue::Command* command) : command(command) {}
};

// Wrapper for Dialogue Choice
struct WrenChoice
{
	Struktur::Dialogue::Choice* choice;

	WrenChoice(Struktur::Dialogue::Choice* choice) : choice(choice) {}
};
