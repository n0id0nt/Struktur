#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Struktur
{
class GameContext;
}

namespace Struktur::Dialogue
{
// Forward declarations
class DialogueValue;
class DialogueRegistry;

// =============================================================================
// Variable Modifier - Base class for all modifiers
// =============================================================================

class VariableModifier
{
   public:
	virtual ~VariableModifier() = default;

	// Apply this modifier to a value
	// value: The input value
	// arg: The argument string (e.g., "2" for round:2)
	// registry: Access to other variables (for cross-references)
	virtual std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                          const DialogueRegistry& dialogueRegistry) const = 0;
};

// =============================================================================
// Variable Template - Stored formatting rules
// =============================================================================

struct VariableTemplate
{
	std::vector<std::string> modifiers;  // List of modifier strings to apply
};

// =============================================================================
// Variable Substitution System - Main system
// =============================================================================

class VariableSubstitutionSystem
{
   public:
	VariableSubstitutionSystem();
	~VariableSubstitutionSystem();

	std::string ApplyModifiers(GameContext& context, const DialogueValue& value, const std::string& modifierChain);

	// Register a custom modifier
	void RegisterModifier(const std::string& name, std::unique_ptr<VariableModifier> modifier);

	// Register a template
	void RegisterTemplate(const std::string& name, const VariableTemplate& templ);
	void RegisterTemplate(const std::string& name, const std::vector<std::string>& modifiers);

	// Get a template (for inspection)
	std::optional<VariableTemplate> GetTemplate(const std::string& name) const;

   private:
	std::map<std::string, std::unique_ptr<VariableModifier>> m_modifiers;
	std::map<std::string, VariableTemplate> m_templates;

	// Apply a chain of modifiers to a value
	std::string ApplyModifierChain(GameContext& context, const DialogueValue& value,
	                               const std::string& modifierChain) const;

	// Apply a single modifier
	std::string ApplySingleModifier(GameContext& context, const DialogueValue& value,
	                                const std::string& modifierStr) const;

	// Register built-in modifiers
	void RegisterBuiltInModifiers();

	friend class TemplateModifier;
};

// =============================================================================
// Built-in Modifiers
// =============================================================================

// Round numbers
class RoundModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Add suffix
class SuffixModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Add prefix
class PrefixModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Pluralization
class PluralModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Boolean aliases
class BoolModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Uppercase
class UpperModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Lowercase
class LowerModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Absolute value
class AbsModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Zero-pad
class PadModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

// Template application
class TemplateModifier : public VariableModifier
{
   public:
	std::string Apply(GameContext& context, const DialogueValue& value, const std::string& arg,
	                  const DialogueRegistry& dialogueRegistry) const override;
};

}  // namespace Struktur::Dialogue
