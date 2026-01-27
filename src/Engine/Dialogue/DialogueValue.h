// DialogueValue.h
// Variant type for storing dialogue parameters
// Part of the Struktur dialogue system

#pragma once

#include <string>

namespace Struktur::Dialogue
{
	// Simple variant for dialogue parameters
	struct DialogueValue
	{
		enum class Type
		{
			STRING,
			INT,
			BOOL,
			DOUBLE
		};

		Type type;
		std::string stringValue;
		int intValue;
		bool boolValue;
		double doubleValue;

		// Constructors for each type
		DialogueValue() : type(Type::STRING), stringValue(""), intValue(0), boolValue(false), doubleValue(0.0) {}
		explicit DialogueValue(const std::string& s) : type(Type::STRING), stringValue(s), intValue(0), boolValue(false), doubleValue(0.0) {}
		explicit DialogueValue(const char* s) : type(Type::STRING), stringValue(s), intValue(0), boolValue(false), doubleValue(0.0) {}
		explicit DialogueValue(int i) : type(Type::INT), stringValue(""), intValue(i), boolValue(false), doubleValue(0.0) {}
		explicit DialogueValue(bool b) : type(Type::BOOL), stringValue(""), intValue(0), boolValue(b), doubleValue(0.0) {}
		explicit DialogueValue(double d) : type(Type::DOUBLE), stringValue(""), intValue(0), boolValue(false), doubleValue(d) {}

		// Conversion helpers
		std::string AsString() const;
		int AsInt() const;
		bool AsBool() const;
		double AsDouble() const;
	};
}