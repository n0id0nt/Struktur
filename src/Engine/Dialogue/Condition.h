#pragma once

#include <string>
#include <unordered_map>

#include "DialogueValue.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur::Callback
{
class ICallback;
}

namespace Struktur::Dialogue
{
// Command that executes by calling a Wren callback
class Condition
{
public:
	// Constructor
	// callback is borrowed from DialogueRegistry, not owned
	Condition(const std::string& key, const std::unordered_map<std::string, DialogueValue>& params);
	~Condition() = default;

	// Execute by calling Wren callback with parameters
	bool Evaluate(GameContext& context) const;
	Callback::ICallback* GetCallback(GameContext& context) const;
	const std::unordered_map<std::string, DialogueValue>& GetParams() const;

	const std::string& GetKey() const
	{
		return m_key;
	}
	void SetKey(const std::string& key)
	{
		m_key = key;
	}

	void SetParameter(const std::string& key, const DialogueValue& value);
	void RemoveParameter(const std::string& key);
	void ChangeParameterKey(const std::string& oldKey, const std::string& newKey);

private:
	std::string m_key;
	std::unordered_map<std::string, DialogueValue> m_params;
};
}  // namespace Struktur::Dialogue
