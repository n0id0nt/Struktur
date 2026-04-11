// DialogueExporter.h
// Exports dialogue nodes back to Wren format
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "nlohmann/json.hpp"

#include "Engine/Dialogue/DialogueNode.h"

namespace Struktur::Dialogue
{
	// Exports dialogue nodes to Wren script format
	class DialogueExporter
	{
	public:
		enum class DialogueSaveFormat
		{
			Wren,
			Json
		};

		// Export entire dialogue to Wren format
		static std::string ExportToWren(
			const std::unordered_map<std::string, Dialogue::DialogueNode*>& nodes,
			const std::string& className);

		// Export entire dialogue to Json format
		static std::string ExportToJson(
			const std::unordered_map<std::string, Dialogue::DialogueNode*>& nodes);

	private:
		// Export individual components
		static std::string ExportChoiceWren(const Choice& choice, int indent);
		static std::string ExportCommandWren(const Command& command, int indent);
		static std::string ExportConditionWren(const Condition& condition, int indent);
		static std::string ExportTargetWren(const ConditionalTarget& target, int indent);

		static nlohmann::json ExportChoiceJson(const Choice& choice);
		static nlohmann::json ExportCommandJson(const Command& command);
		static nlohmann::json ExportConditionJson(const Condition& condition);
		static nlohmann::json ExportTargetJson(const ConditionalTarget& target);

		// Helper for indentation
		static std::string Indent(int level);

		// Escape string for Wren
		static std::string EscapeString(const std::string& str);
	};
}
