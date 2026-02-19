#pragma once

#include "DialogueValue.h"
#include <unordered_map>
#include <string>

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
	class Command
	{
	public:
		// Constructor
		// callback is borrowed from DialogueRegistry, not owned
		Command(const std::string& key, const std::unordered_map<std::string, DialogueValue>& params);
		~Command() = default;

		// Execute by calling Wren callback with parameters
		void Execute(GameContext& context) const;
		Callback::ICallback* GetCallback(GameContext& context) const;
		const std::unordered_map<std::string, DialogueValue>& GetParams() const;
		const std::string& GetKey() const { return m_key; }

	private:
		std::string m_key;
		std::unordered_map<std::string, DialogueValue> m_params;

	};
}