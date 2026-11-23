// DialogueExporter.h
// Exports dialogue nodes back to Wren format
// Part of the Struktur dialogue system

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Dialogue/DialogueNode.h"
#include "glm/glm.hpp"
#include "nlohmann/json.hpp"

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
	static std::string ExportToWren(const std::unordered_map<std::string, Dialogue::DialogueNode*>& nodes,
	                                const std::string& className);

	// Export entire dialogue to Json format: {"root": rootNode, "nodes": [...] in the given order, "editor":
	// {"positions": {node id: [x, y]}}} - the root is what the dialogue editor opens on, the positions where its graph
	// draws each node (the game only reads "nodes").
	static std::string ExportToJson(const std::vector<std::pair<std::string, Dialogue::DialogueNode*>>& orderedNodes,
	                                const std::string& rootNode,
	                                const std::unordered_map<std::string, glm::vec2>& positions);

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
}  // namespace Struktur::Dialogue
