#pragma once

#include <string>
#include <variant>

#include "nlohmann/json.hpp"

namespace Struktur::Dialogue
{
// Simple variant for dialogue parameters
class DialogueValue
{
   public:
	using ValueVariant = std::variant<std::string, int, bool, double>;

	// Constructors for each type
	DialogueValue()
	    : m_value(std::string(""))
	{
	}
	explicit DialogueValue(const std::string& s)
	    : m_value(s)
	{
	}
	explicit DialogueValue(const char* s)
	    : m_value(std::string(s))
	{
	}
	explicit DialogueValue(int i)
	    : m_value(i)
	{
	}
	explicit DialogueValue(bool b)
	    : m_value(b)
	{
	}
	explicit DialogueValue(double d)
	    : m_value(d)
	{
	}

	// Conversion helpers
	std::string ExportValue() const;
	std::string AsString() const;
	int AsInt() const;
	bool AsBool() const;
	double AsDouble() const;
	nlohmann::json AsJson() const;

	// Type checking
	bool IsString() const
	{
		return std::holds_alternative<std::string>(m_value);
	}
	bool IsInt() const
	{
		return std::holds_alternative<int>(m_value);
	}
	bool IsBool() const
	{
		return std::holds_alternative<bool>(m_value);
	}
	bool IsDouble() const
	{
		return std::holds_alternative<double>(m_value);
	}

	// Direct variant access if needed
	const ValueVariant& GetVariant() const
	{
		return m_value;
	}

   private:
	ValueVariant m_value;
};
}  // namespace Struktur::Dialogue
