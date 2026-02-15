#include "DialogueExporter.h"

#include "Debug/Assertions.h"
#include <sstream>

namespace Struktur::Dialogue
{
	std::string DialogueExporter::ExportToWren(
		const std::map<std::string, std::unique_ptr<DialogueNode>>& nodes,
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
				if (hasData) output << ",\n";
				output << Indent(5) << "\"speaker\": \"" << EscapeString(node->GetSpeaker().value()) << "\"";
				hasData = true;
			}

			// Text
			if (node->GetText().has_value())
			{
				if (hasData) output << ",\n";
				output << Indent(5) << "\"text\": \"" << EscapeString(node->GetText().value()) << "\"";
				hasData = true;
			}

			// Next
			if (node->GetNext().has_value())
			{
				if (hasData) output << ",\n";
				output << Indent(5) << "\"next\": \"" << EscapeString(node->GetNext().value()) << "\"";
				hasData = true;
			}

			// Commands
			if (!node->GetCommands().empty())
			{
				if (hasData) output << ",\n";
				output << Indent(5) << "\"commands\": [\n";

				bool firstCmd = true;
				for (const auto& command : node->GetCommands())
				{
					if (!firstCmd) output << ",\n";
					firstCmd = false;
					output << ExportCommand(*command, 6);
				}

				output << "\n" << Indent(5) << "]";
				hasData = true;
			}

			// Choices
			if (!node->GetChoices().empty())
			{
				if (hasData) output << ",\n";
				output << Indent(5) << "\"choices\": [\n";

				bool firstChoice = true;
				for (const auto& choice : node->GetChoices())
				{
					if (!firstChoice) output << ",\n";
					firstChoice = false;
					output << ExportChoice(choice, 6);
				}

				output << "\n" << Indent(5) << "]";
				hasData = true;
			}

			// Targets
			if (!node->GetTargets().empty())
			{
				if (hasData) output << ",\n";
				output << Indent(5) << "\"targets\": [\n";

				bool firstTarget = true;
				for (const auto& target : node->GetTargets())
				{
					if (!firstTarget) output << ",\n";
					firstTarget = false;
					output << ExportTarget(target, 6);
				}

				output << "\n" << Indent(5) << "]";
				hasData = true;
			}

			// Close node data
			if (hasData) output << "\n";
			output << Indent(4) << "}\n";
			output << Indent(3) << "}";
		}

		// Close class
		output << "\n" << Indent(2) << "]\n";
		output << Indent(1) << "}\n";
		output << "}\n";

		return output.str();
	}

	std::string DialogueExporter::ExportChoice(const Choice& choice, int indent)
	{
		std::ostringstream output;
		output << Indent(indent) << "{\"text\": \"" << EscapeString(choice.text)
			   << "\", \"target\": \"" << EscapeString(choice.targetNode) << "\"}";
		return output.str();
	}

	std::string DialogueExporter::ExportCommand(const Command& command, int indent)
	{
		std::ostringstream output;
		output << Indent(indent) << "{\"type\": \"" << EscapeString(command.GetType()) << "\", ";
		output << "\"parameters\": {";

		// Export parameters
		// Note: This assumes Command has a GetParameters() method
		// You'll need to add this to Command interface or use a different approach
		const auto& params = command.GetParameters();
		bool first = true;
		for (const auto& [key, value] : params)
		{
			if (!first) output << ", ";
			first = false;

			output << "\"" << EscapeString(key) << "\": ";

			switch (value.type)
			{
				case DialogueValue::Type::STRING:
					output << "\"" << EscapeString(value.stringValue) << "\"";
					break;
				case DialogueValue::Type::INT:
					output << value.intValue;
					break;
				case DialogueValue::Type::BOOL:
					output << (value.boolValue ? "true" : "false");
					break;
				case DialogueValue::Type::DOUBLE:
					output << value.doubleValue;
					break;
			}
		}

		output << "}}";
		return output.str();
	}

	std::string DialogueExporter::ExportCondition(const Condition& condition, int indent)
	{
		std::ostringstream output;
		output << Indent(indent) << "{\"type\": \"" << EscapeString(condition.GetType()) << "\", ";
		output << "\"parameters\": {";

		// Export parameters
		const auto& params = condition.GetParameters();
		bool first = true;
		for (const auto& [key, value] : params)
		{
			if (!first) output << ", ";
			first = false;

			output << "\"" << EscapeString(key) << "\": ";

			switch (value.type)
			{
				case DialogueValue::Type::STRING:
					output << "\"" << EscapeString(value.stringValue) << "\"";
					break;
				case DialogueValue::Type::INT:
					output << value.intValue;
					break;
				case DialogueValue::Type::BOOL:
					output << (value.boolValue ? "true" : "false");
					break;
				case DialogueValue::Type::DOUBLE:
					output << value.doubleValue;
					break;
			}
		}

		output << "}}";
		return output.str();
	}

	std::string DialogueExporter::ExportTarget(const ConditionalTarget& target, int indent)
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
				if (!first) output << ",\n";
				first = false;
				output << ExportCondition(*condition, indent + 2);
			}

			output << "\n" << Indent(indent + 1) << "],\n";
		}

		// Export target node
		output << Indent(indent + 1) << "\"node\": \"" << EscapeString(target.targetNode) << "\"\n";
		output << Indent(indent) << "}";

		return output.str();
	}

	std::string DialogueExporter::Indent(int level)
	{
		return std::string(level * 4, ' ');
	}

	std::string DialogueExporter::EscapeString(const std::string& str)
	{
		std::string result;
		result.reserve(str.size());

		for (char c : str)
		{
			switch (c)
			{
				case '"':  result += "\\\""; break;
				case '\\': result += "\\\\"; break;
				case '\n': result += "\\n"; break;
				case '\r': result += "\\r"; break;
				case '\t': result += "\\t"; break;
				default:   result += c; break;
			}
		}

		return result;
	}
}
