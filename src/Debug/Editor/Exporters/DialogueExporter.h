// DialogueExporter.h
// Exports dialogue nodes back to Wren format
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "Engine/Dialogue/DialogueNode.h"

namespace Struktur::Dialogue
{
	// Exports dialogue nodes to Wren script format
	class DialogueExporter
	{
	public:
		// Export entire dialogue to Wren format
		static std::string ExportToWren(
			const std::unordered_map<std::string, Dialogue::DialogueNode*>& nodes,
			const std::string& className);

	private:
		// Export individual components
		static std::string ExportNode(const DialogueNode& node);
		static std::string ExportChoice(const Choice& choice, int indent);
		static std::string ExportCommand(const Command& command, int indent);
		static std::string ExportCondition(const Condition& condition, int indent);
		static std::string ExportTarget(const ConditionalTarget& target, int indent);

		// Helper for indentation
		static std::string Indent(int level);

		// Escape string for Wren
		static std::string EscapeString(const std::string& str);
	};
}
