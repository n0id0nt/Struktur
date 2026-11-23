#include "DialogueExporter.h"

#include <cmath>
#include <sstream>

#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
std::string DialogueExporter::ExportToWren(const std::unordered_map<std::string, Dialogue::DialogueNode*>& nodes,
                                           const std::string& className)
{
	std::ostringstream output;

	// Class header
	output << "class " << className << " {\n";
	output << Indent(1) << "static getData() {\n";
	output << Indent(2) << "return [\n";

	// Export each node
	bool first = true;
	for (const auto& [nodeId, node] : nodes)
	{
		if (!first)
		{
			output << ",\n";
		}
		first = false;

		output << Indent(3) << "{ \n";
		output << Indent(4) << "\"node\": \"" << EscapeString(nodeId) << "\", \"data\": {\n";

		// Export node data
		bool hasData = false;

		// Speaker
		if (node->GetSpeaker().has_value())
		{
			if (hasData)
			{
				output << ",\n";
			}
			output << Indent(5) << "\"speaker\": \"" << EscapeString(node->GetSpeaker().value()) << "\"";
			hasData = true;
		}

		// Text
		if (node->GetText().has_value())
		{
			if (hasData)
			{
				output << ",\n";
			}
			output << Indent(5) << "\"text\": \"" << EscapeString(node->GetText().value()) << "\"";
			hasData = true;
		}

		// Next
		if (node->GetNext().has_value())
		{
			if (hasData)
			{
				output << ",\n";
			}
			output << Indent(5) << "\"next\": \"" << EscapeString(node->GetNext().value()) << "\"";
			hasData = true;
		}

		// Commands
		if (!node->GetCommands().empty())
		{
			if (hasData)
			{
				output << ",\n";
			}
			output << Indent(5) << "\"commands\": [\n";

			bool firstCmd = true;
			for (const auto& command : node->GetCommands())
			{
				if (!firstCmd)
				{
					output << ",\n";
				}
				firstCmd = false;
				output << ExportCommandWren(*command, 6);
			}

			output << "\n" << Indent(5) << "]";
			hasData = true;
		}

		// Choices
		if (!node->GetChoices().empty())
		{
			if (hasData)
			{
				output << ",\n";
			}
			output << Indent(5) << "\"choices\": [\n";

			bool firstChoice = true;
			for (const auto& choice : node->GetChoices())
			{
				if (!firstChoice)
				{
					output << ",\n";
				}
				firstChoice = false;
				output << ExportChoiceWren(*choice.get(), 6);
			}

			output << "\n" << Indent(5) << "]";
			hasData = true;
		}

		// Targets
		if (!node->GetTargets().empty())
		{
			if (hasData)
			{
				output << ",\n";
			}
			output << Indent(5) << "\"targets\": [\n";

			bool firstTarget = true;
			for (const auto& target : node->GetTargets())
			{
				if (!firstTarget)
				{
					output << ",\n";
				}
				firstTarget = false;
				output << ExportTargetWren(*target.get(), 6);
			}

			output << "\n" << Indent(5) << "]";
			hasData = true;
		}

		// Close node data
		if (hasData)
		{
			output << "\n";
		}
		output << Indent(4) << "}\n";
		output << Indent(3) << "}";
	}

	// Close class
	output << "\n" << Indent(2) << "]\n";
	output << Indent(1) << "}\n";
	output << "}\n";

	return output.str();
}

std::string DialogueExporter::ExportToJson(const std::vector<std::pair<std::string, Dialogue::DialogueNode*>>& orderedNodes,
                                          const std::string& rootNode,
                                          const std::unordered_map<std::string, glm::vec2>& positions)
{
	nlohmann::json nodeList = nlohmann::json::array();

	for (const auto& [nodeId, node] : orderedNodes)
	{
		nlohmann::json nodeData = nlohmann::json::object();

		// Speaker
		if (node->GetSpeaker().has_value())
		{
			nodeData["speaker"] = node->GetSpeaker().value();
		}

		// Text
		if (node->GetText().has_value())
		{
			nodeData["text"] = node->GetText().value();
		}

		// Next
		if (node->GetNext().has_value())
		{
			nodeData["next"] = node->GetNext().value();
		}

		// Commands
		if (!node->GetCommands().empty())
		{
			nlohmann::json commands = nlohmann::json::array();
			for (const auto& command : node->GetCommands())
			{
				commands.push_back(ExportCommandJson(*command));
			}
			nodeData["commands"] = commands;
		}

		// Choices
		if (!node->GetChoices().empty())
		{
			nlohmann::json choices = nlohmann::json::array();
			for (const auto& choice : node->GetChoices())
			{
				choices.push_back(ExportChoiceJson(*choice.get()));
			}
			nodeData["choices"] = choices;
		}

		// Targets
		if (!node->GetTargets().empty())
		{
			nlohmann::json targets = nlohmann::json::array();
			for (const auto& target : node->GetTargets())
			{
				targets.push_back(ExportTargetJson(*target.get()));
			}
			nodeData["targets"] = targets;
		}

		nodeList.push_back({{"node", nodeId}, {"data", nodeData}});
	}

	// Whole pixels keep the file readable and stable between saves.
	nlohmann::json positionMap = nlohmann::json::object();
	for (const auto& [nodeId, node] : orderedNodes)
	{
		auto found = positions.find(nodeId);
		if (found != positions.end())
		{
			positionMap[nodeId] = {std::round(found->second.x), std::round(found->second.y)};
		}
	}

	nlohmann::json document = nlohmann::json::object();
	document["root"]        = rootNode;
	document["nodes"]       = nodeList;
	document["editor"]      = {{"positions", positionMap}};
	return document.dump(2) + "\n";
}

std::string DialogueExporter::ExportChoiceWren(const Choice& choice, int indent)
{
	std::ostringstream output;
	output << Indent(indent) << "{\"text\": \"" << EscapeString(choice.text) << "\", \"target\": \""
	       << EscapeString(choice.targetNode) << "\"}";
	return output.str();
}

std::string DialogueExporter::ExportCommandWren(const Command& command, int indent)
{
	std::ostringstream output;
	output << Indent(indent) << "{\n";
	output << Indent(indent + 1) << "\"type\": \"" << EscapeString(command.GetKey()) << "\", ";
	output << "\"parameters\": [\n";

	// Export parameters
	// Note: This assumes Command has a GetParameters() method
	// You'll need to add this to Command interface or use a different approach
	const auto& params = command.GetParams();
	bool first         = true;
	for (const auto& [key, value] : params)
	{
		if (!first)
		{
			output << ",\n";
		}
		first = false;

		output << Indent(indent + 2) << "{\n";

		output << Indent(indent + 3) << "\"type\": \"" << EscapeString(key) << "\",\n";
		output << Indent(indent + 3) << "\"value\": " << value.ExportValue() << "\n";

		output << Indent(indent + 2) << "}";
	}

	output << "\n" << Indent(indent + 1) << "]\n";
	output << Indent(indent) << "}";
	return output.str();
}

std::string DialogueExporter::ExportConditionWren(const Condition& condition, int indent)
{
	std::ostringstream output;
	output << Indent(indent) << "{\n";
	output << Indent(indent + 1) << "\"type\": \"" << EscapeString(condition.GetKey()) << "\", ";
	output << "\"parameters\": [\n";

	// Export parameters
	const auto& params = condition.GetParams();
	bool first         = true;
	for (const auto& [key, value] : params)
	{
		if (!first)
		{
			output << ",\n";
		}
		first = false;

		output << Indent(indent + 2) << "{\n";

		output << Indent(indent + 3) << "\"type\": \"" << EscapeString(key) << "\",\n";
		output << Indent(indent + 3) << "\"value\": " << value.ExportValue() << "\n";

		output << Indent(indent + 2) << "}";
	}

	output << "\n" << Indent(indent + 1) << "]\n";
	output << Indent(indent) << "}";
	return output.str();
}

std::string DialogueExporter::ExportTargetWren(const ConditionalTarget& target, int indent)
{
	std::ostringstream output;
	output << Indent(indent) << "{\n";

	// Export conditions
	if (!target.conditions.empty())
	{
		output << Indent(indent + 1) << "\"conditions\": [\n";

		bool first = true;
		for (const auto& condition : target.conditions)
		{
			if (!first)
			{
				output << ",\n";
			}
			first = false;
			output << ExportConditionWren(*condition, indent + 2);
		}

		output << "\n" << Indent(indent + 1) << "],\n";
	}

	// Export target node
	output << Indent(indent + 1) << "\"node\": \"" << EscapeString(target.targetNode) << "\"\n";
	output << Indent(indent) << "}";

	return output.str();
}

nlohmann::json DialogueExporter::ExportChoiceJson(const Choice& choice)
{
	return {{"text", choice.text}, {"target", choice.targetNode}};
}

nlohmann::json DialogueExporter::ExportCommandJson(const Command& command)
{
	nlohmann::json params = nlohmann::json::array();
	for (const auto& [key, value] : command.GetParams())
	{
		params.push_back({{"type", key}, {"value", value.AsJson()}});
	}

	return {{"type", command.GetKey()}, {"parameters", params}};
}

nlohmann::json DialogueExporter::ExportConditionJson(const Condition& condition)
{
	nlohmann::json params = nlohmann::json::array();
	for (const auto& [key, value] : condition.GetParams())
	{
		params.push_back({{"type", key}, {"value", value.AsJson()}});
	}

	return {{"type", condition.GetKey()}, {"parameters", params}};
}

nlohmann::json DialogueExporter::ExportTargetJson(const ConditionalTarget& target)
{
	nlohmann::json j = nlohmann::json::object();

	if (!target.conditions.empty())
	{
		nlohmann::json conditions = nlohmann::json::array();
		for (const auto& condition : target.conditions)
		{
			conditions.push_back(ExportConditionJson(*condition));
		}
		j["conditions"] = conditions;
	}

	j["node"] = target.targetNode;
	return j;
}

std::string DialogueExporter::Indent(int level)
{
	return std::string(level * 4, ' ');
}

std::string DialogueExporter::EscapeString(const std::string& str)
{
	std::string result;
	result.reserve(str.size());

	for (char character : str)
	{
		switch (character)
		{
			case '"':
				result += "\\\"";
				break;
			case '\\':
				result += "\\\\";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
			default:
				result += character;
				break;
		}
	}

	return result;
}
}  // namespace Struktur::Dialogue
