#include "DialogueRegistry.h"

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"

namespace Struktur::Dialogue
{
	DialogueRegistry::DialogueRegistry()
		: m_conditionCallbacks()
		, m_commandCallbacks()
	{
	}

	DialogueRegistry::~DialogueRegistry()
	{

	}

    void DialogueRegistry::Clear(GameContext &context)
    {
		DEBUG_INFO("DialogueRegistry destroyed, releasing %zu condition callbacks and %zu command callbacks", m_conditionCallbacks.size(), m_commandCallbacks.size());

		// Release all condition callbacks
		for (auto& [type, callback] : m_conditionCallbacks)
		{
			if (callback)
				callback->Dispose(context);
		}
		m_conditionCallbacks.clear();

		// Release all command callbacks
		for (auto& [type, callback] : m_commandCallbacks)
		{
			if (callback)
				callback->Dispose(context);
		}
		m_commandCallbacks.clear();
    }

    void DialogueRegistry::RegisterConditionType(GameContext& context, const std::string& type, std::unique_ptr<Callback::ICallback> callback)
	{
		// If type already exists, release old handle
		auto it = m_conditionCallbacks.find(type);
		if (it != m_conditionCallbacks.end())
		{
			DEBUG_WARNING("Condition type '%s' already registered, replacing", type.c_str());
			it->second->Dispose(context);
		}

		m_conditionCallbacks[type] = std::move(callback);
		DEBUG_INFO("Registered condition type: %s", type.c_str());
	}

	void DialogueRegistry::RegisterCommandType(GameContext& context, const std::string& type, std::unique_ptr<Callback::ICallback> callback)
	{
		// If type already exists, release old handle
		auto it = m_commandCallbacks.find(type);
		if (it != m_commandCallbacks.end())
		{
			DEBUG_WARNING("Command type '%s' already registered, replacing", type.c_str());
			it->second->Dispose(context);
		}

		m_commandCallbacks[type] = std::move(callback);
		DEBUG_INFO("Registered command type: %s", type.c_str());
	}

	bool DialogueRegistry::HasConditionType(const std::string& type) const
	{
		return m_conditionCallbacks.find(type) != m_conditionCallbacks.end();
	}

	bool DialogueRegistry::HasCommandType(const std::string& type) const
	{
		return m_commandCallbacks.find(type) != m_commandCallbacks.end();
	}

    Callback::ICallback* DialogueRegistry::GetCondition(const std::string &type)
    {
        return m_conditionCallbacks[type].get();
    }

    Callback::ICallback* DialogueRegistry::GetCommand(const std::string &type)
    {
        return m_commandCallbacks[type].get();
    }
}
