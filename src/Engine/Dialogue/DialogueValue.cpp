#include "DialogueValue.h"

namespace Struktur::Dialogue
{
	std::string DialogueValue::AsString() const
	{
		switch (type)
		{
			case Type::STRING: return stringValue;
			case Type::INT: return std::to_string(intValue);
			case Type::BOOL: return boolValue ? "true" : "false";
			case Type::DOUBLE: return std::to_string(doubleValue);
		}
		return "";
	}

	int DialogueValue::AsInt() const
	{
		switch (type)
		{
			case Type::STRING: return std::stoi(stringValue);
			case Type::INT: return intValue;
			case Type::BOOL: return boolValue ? 1 : 0;
			case Type::DOUBLE: return static_cast<int>(doubleValue);
		}
		return 0;
	}

	bool DialogueValue::AsBool() const
	{
		switch (type)
		{
			case Type::STRING: return !stringValue.empty();
			case Type::INT: return intValue != 0;
			case Type::BOOL: return boolValue;
			case Type::DOUBLE: return doubleValue != 0.0;
		}
		return false;
	}

	double DialogueValue::AsDouble() const
	{
		switch (type)
		{
			case Type::STRING: return std::stod(stringValue);
			case Type::INT: return static_cast<double>(intValue);
			case Type::BOOL: return boolValue ? 1.0 : 0.0;
			case Type::DOUBLE: return doubleValue;
		}
		return 0.0;
	}
}