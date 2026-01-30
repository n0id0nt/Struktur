#include "DialogueRegistry.h"

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"

namespace Struktur::Dialogue
{
	DialogueRegistry::DialogueRegistry()
		: m_conditionCallbacks()
		, m_commandCallbacks()
		, m_operatorCallbacks()
	{
	}

	DialogueRegistry::~DialogueRegistry()
	{

	}

    void DialogueRegistry::Clear(GameContext &context)
    {
		DEBUG_INFO("DialogueRegistry destroyed, releasing %zu condition callbacks, %zu command callbacks, %zu operator callbacks", m_conditionCallbacks.size(), m_commandCallbacks.size(), m_operatorCallbacks.size());

		// Release all condition callbacks
		for (auto& [type, callback] : m_conditionCallbacks)
		{
			callback.disposeCallBack(context);
		}
		m_conditionCallbacks.clear();

		// Release all command callbacks
		for (auto& [type, callback] : m_commandCallbacks)
		{
			callback.disposeCallBack(context);
		}
		m_commandCallbacks.clear();

		// Release all operator callbacks
		for (auto& [op, callback] : m_operatorCallbacks)
		{
			callback.disposeCallBack(context);
		}
		m_operatorCallbacks.clear();
    }

    void DialogueRegistry::RegisterConditionType(GameContext& context, const std::string& type, ConditionCallback && callback, DisposeCallback && DisposeCallback)
	{
		// If type already exists, release old handle
		auto it = m_conditionCallbacks.find(type);
		if (it != m_conditionCallbacks.end())
		{
			DEBUG_WARNING("Condition type '%s' already registered, replacing", type.c_str());
			it->second.disposeCallBack(context);
		}

		m_conditionCallbacks[type] = {std::move(callback), std::move(DisposeCallback)};
		DEBUG_INFO("Registered condition type: %s", type.c_str());
	}

	void DialogueRegistry::RegisterCommandType(GameContext& context, const std::string& type, CommandCallback && callback, DisposeCallback && DisposeCallback)
	{
		// If type already exists, release old handle
		auto it = m_commandCallbacks.find(type);
		if (it != m_commandCallbacks.end())
		{
			DEBUG_WARNING("Command type '%s' already registered, replacing", type.c_str());
			it->second.disposeCallBack(context);
		}

		m_commandCallbacks[type] = {std::move(callback), std::move(DisposeCallback)};
		DEBUG_INFO("Registered command type: %s", type.c_str());
	}

	void DialogueRegistry::RegisterOperator(GameContext& context, const std::string& op, OperatorCallback && callback, DisposeCallback && DisposeCallback)
	{
		// If operator already exists, release old handle
		auto it = m_operatorCallbacks.find(op);
		if (it != m_operatorCallbacks.end())
		{
			DEBUG_WARNING("Operator '%s' already registered, replacing", op.c_str());
			it->second.disposeCallBack(context);
		}

		m_operatorCallbacks[op] = {std::move(callback), std::move(DisposeCallback)};
		DEBUG_INFO("Registered operator: %s", op.c_str());
	}

	bool DialogueRegistry::EvaluateOperator(GameContext& context, const std::string& op, const DialogueValue& lhs, const DialogueValue& rhs)
	{
		auto it = m_operatorCallbacks.find(op);
		if (it == m_operatorCallbacks.end())
		{
			DEBUG_ERROR("Operator '%s' not registered", op.c_str());
			return false;
		}

		return it->second.callback(lhs, rhs);
	}

	bool DialogueRegistry::HasConditionType(const std::string& type) const
	{
		return m_conditionCallbacks.find(type) != m_conditionCallbacks.end();
	}

	bool DialogueRegistry::HasCommandType(const std::string& type) const
	{
		return m_commandCallbacks.find(type) != m_commandCallbacks.end();
	}

	bool DialogueRegistry::HasOperator(const std::string& op) const
	{
		return m_operatorCallbacks.find(op) != m_operatorCallbacks.end();
	}

    ConditionCallback& DialogueRegistry::GetCondition(const std::string &type)
    {
        return m_conditionCallbacks[type].callback;
    }

    CommandCallback& DialogueRegistry::GetCommand(const std::string &type)
    {
        return m_commandCallbacks[type].callback;
    }

    OperatorCallback& DialogueRegistry::GetOperator(const std::string &op)
    {
        return m_operatorCallbacks[op].callback;
    }
}
