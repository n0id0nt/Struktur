#include "VariableSubstitutionSystem.h"

#include <regex>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueValue.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/DialogueHelperFunctions.h"

namespace Struktur::Dialogue
{
	// =============================================================================
	// VariableSubstitutionSystem - Implementation
	// =============================================================================

	VariableSubstitutionSystem::VariableSubstitutionSystem()
	{
		RegisterBuiltInModifiers();
	}

	VariableSubstitutionSystem::~VariableSubstitutionSystem() = default;

	std::string VariableSubstitutionSystem::ApplyModifiers(GameContext& context, const DialogueValue& value, const std::string& modifierChain)
	{
		return ApplyModifierChain(context, value, modifierChain);
	}

	void VariableSubstitutionSystem::RegisterModifier(const std::string& name,
		std::unique_ptr<VariableModifier> modifier)
	{
		m_modifiers[name] = std::move(modifier);
	}

	void VariableSubstitutionSystem::RegisterTemplate(const std::string& name,
		const VariableTemplate& templ)
	{
		m_templates[name] = templ;
	}

	void VariableSubstitutionSystem::RegisterTemplate(const std::string& name,
		const std::vector<std::string>& modifiers)
	{
		VariableTemplate templ;
		templ.modifiers = modifiers;
		m_templates[name] = templ;
	}

	std::optional<VariableTemplate> VariableSubstitutionSystem::GetTemplate(const std::string& name) const
	{
		auto it = m_templates.find(name);
		return (it != m_templates.end()) ? std::optional(it->second) : std::nullopt;
	}

	std::string VariableSubstitutionSystem::ApplyModifierChain(
		GameContext& context,
		const DialogueValue& value,
		const std::string& modifierChain) const
	{
		if (modifierChain.empty())
		{
			return value.AsString();
		}

		// Split by pipe, but be careful of pipes inside arguments
		std::vector<std::string> modifiers;
		std::string current;
		int depth = 0;

		for (char c : modifierChain)
		{
			if (c == '|' && depth == 0)
			{
				if (!current.empty())
				{
					modifiers.push_back(current);
					current.clear();
				}
			}
			else
			{
				if (c == '(') depth++;
				if (c == ')') depth--;
				current += c;
			}
		}

		if (!current.empty())
		{
			modifiers.push_back(current);
		}

		// Apply each modifier in sequence
		DialogueValue result = value;
		std::string resultStr = value.AsString();

		for (const auto& modStr : modifiers)
		{
			resultStr = ApplySingleModifier(context, DialogueValue(resultStr), modStr);
		}

		return resultStr;
	}

	std::string VariableSubstitutionSystem::ApplySingleModifier(
		GameContext& context,
		const DialogueValue& value,
		const std::string& modifierStr) const
	{
		// Parse modifier: "name:arg"
		size_t colonPos = modifierStr.find(':');
		std::string modName = (colonPos != std::string::npos)
			? modifierStr.substr(0, colonPos)
			: modifierStr;
		std::string modArg = (colonPos != std::string::npos)
			? modifierStr.substr(colonPos + 1)
			: "";

		// Trim whitespace
		modName.erase(0, modName.find_first_not_of(" \t"));
		modName.erase(modName.find_last_not_of(" \t") + 1);

		// Find and apply modifier
		auto it = m_modifiers.find(modName);
		if (it != m_modifiers.end())
		{
			auto& dialogueRegistry = context.GetDialogueRegistry();
			return it->second->Apply(context, value, modArg, dialogueRegistry);
		}

		// Unknown modifier - return value as-is
		return value.AsString();
	}

	void VariableSubstitutionSystem::RegisterBuiltInModifiers()
	{
		// Numeric modifiers
		RegisterModifier("round", std::make_unique<RoundModifier>());
		RegisterModifier("abs", std::make_unique<AbsModifier>());
		RegisterModifier("pad", std::make_unique<PadModifier>());

		// String modifiers
		RegisterModifier("suffix", std::make_unique<SuffixModifier>());
		RegisterModifier("prefix", std::make_unique<PrefixModifier>());
		RegisterModifier("upper", std::make_unique<UpperModifier>());
		RegisterModifier("lower", std::make_unique<LowerModifier>());

		// Pluralization
		RegisterModifier("plural", std::make_unique<PluralModifier>());

		// Boolean
		RegisterModifier("bool", std::make_unique<BoolModifier>());

		// Template application
		RegisterModifier("template", std::make_unique<TemplateModifier>());
	}

	// =============================================================================
	// RoundModifier
	// =============================================================================

	std::string RoundModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		double num = value.AsDouble();
		int decimals = arg.empty() ? 0 : std::stoi(arg);

		if (decimals == 0)
		{
			return std::to_string(static_cast<int>(std::round(num)));
		}
		else
		{
			double multiplier = std::pow(10.0, decimals);
			double rounded = std::round(num * multiplier) / multiplier;

			std::stringstream ss;
			ss << std::fixed << std::setprecision(decimals) << rounded;
			return ss.str();
		}
	}

	// =============================================================================
	// SuffixModifier
	// =============================================================================

	std::string SuffixModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		return value.AsString() + arg;
	}

	// =============================================================================
	// PrefixModifier
	// =============================================================================

	std::string PrefixModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		return arg + value.AsString();
	}

	// =============================================================================
	// PluralModifier
	// =============================================================================

	std::string PluralModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		// Parse arg: "singular/plural" or "singular/plural/zero" or "singular/plural:countVar"
		std::string argStr = arg;
		std::string countVarName;

		// Check for count variable reference
		size_t colonPos = argStr.find(':');
		if (colonPos != std::string::npos)
		{
			countVarName = argStr.substr(colonPos + 1);
			argStr = argStr.substr(0, colonPos);
		}

		// Parse forms
		std::vector<std::string> forms;
		size_t pos = 0;
		while (pos < argStr.length())
		{
			size_t slashPos = argStr.find('/', pos);
			if (slashPos == std::string::npos)
			{
				forms.push_back(argStr.substr(pos));
				break;
			}
			forms.push_back(argStr.substr(pos, slashPos - pos));
			pos = slashPos + 1;
		}

		if (forms.empty())
		{
			return value.AsString();
		}

		std::string singular = forms[0];
		std::string plural = (forms.size() > 1) ? forms[1] : singular + "s";
		std::string zeroForm = (forms.size() > 2) ? forms[2] : plural;

		// Determine count
		double count;
		if (!countVarName.empty())
		{
			auto callback = dialogueRegistry.GetVariable(countVarName);
			auto result = callback->Invoke(context, {});
			DialogueValue value = HelperFunctions::ConvertVariantToDialogueValue(result);
			ASSERT_MSG(value.IsDouble() || value.AsInt(), "Dialogue Value %s must be a number", value.AsString());
		}
		else
		{
			count = value.AsDouble();
		}

		// Format with count and form
		std::stringstream ss;

		if (std::abs(count) < 0.001)  // Zero
		{
			ss << static_cast<int>(count) << " " << zeroForm;
		}
		else if (std::abs(count - 1.0) < 0.001 || std::abs(count + 1.0) < 0.001)  // 1 or -1
		{
			ss << static_cast<int>(count) << " " << singular;
		}
		else
		{
			// Check if it's a whole number
			if (std::abs(count - std::round(count)) < 0.001)
			{
				ss << static_cast<int>(std::round(count)) << " " << plural;
			}
			else
			{
				ss << std::fixed << std::setprecision(1) << count << " " << plural;
			}
		}

		return ss.str();
	}

	// =============================================================================
	// BoolModifier
	// =============================================================================

	std::string BoolModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		// Parse arg: "trueStr/falseStr"
		size_t slashPos = arg.find('/');
		std::string trueStr = (slashPos != std::string::npos) ? arg.substr(0, slashPos) : "true";
		std::string falseStr = (slashPos != std::string::npos) ? arg.substr(slashPos + 1) : "false";

		bool boolValue = value.AsBool();
		return boolValue ? trueStr : falseStr;
	}

	// =============================================================================
	// UpperModifier
	// =============================================================================

	std::string UpperModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		std::string str = value.AsString();
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	// =============================================================================
	// LowerModifier
	// =============================================================================

	std::string LowerModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		std::string str = value.AsString();
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	// =============================================================================
	// AbsModifier
	// =============================================================================

	std::string AbsModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		double num = value.AsDouble();
		return std::to_string(static_cast<int>(std::abs(num)));
	}

	// =============================================================================
	// PadModifier
	// =============================================================================

	std::string PadModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		int width = arg.empty() ? 0 : std::stoi(arg);
		std::string str = value.AsString();

		if (static_cast<int>(str.length()) >= width)
		{
			return str;
		}

		return std::string(width - str.length(), '0') + str;
	}

	// =============================================================================
	// TemplateModifier
	// =============================================================================

	std::string TemplateModifier::Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
		const DialogueRegistry& dialogueRegistry) const
	{
		if (arg.empty())
		{
			return value.AsString();
		}

		auto& variableSubstitutionSystem = context.GetVariableSubstitutionSystem();

		// Get template
		auto templ = variableSubstitutionSystem.GetTemplate(arg);
		if (!templ.has_value())
		{
			return value.AsString();
		}

		// Apply template's modifiers
		DialogueValue result = value;
		std::string resultStr = value.AsString();

		for (const auto& modStr : templ->modifiers)
		{
			resultStr = variableSubstitutionSystem.ApplySingleModifier(context, DialogueValue(resultStr), modStr);
		}

		return resultStr;
	}

} // namespace Struktur::Dialogue