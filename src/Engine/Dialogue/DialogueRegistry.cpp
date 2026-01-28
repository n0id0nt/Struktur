#include "DialogueRegistry.h"

#include "WrenCondition.h"
#include "WrenCommand.h"
#include "wren.h"
#include "Debug/Assertions.h"
#include "Engine/GameContext.h"

namespace Struktur::Dialogue
{
	DialogueRegistry::DialogueRegistry()
		: m_conditionCallbacks()
		, m_commandCallbacks()
		, m_operatorCallbacks()
	{
		DEBUG_INFO("DialogueRegistry initialized");
	}

	DialogueRegistry::~DialogueRegistry()
	{

	}

    void DialogueRegistry::Clear(GameContext &context)
    {
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		DEBUG_INFO("DialogueRegistry destroyed, releasing %zu condition callbacks, %zu command callbacks, %zu operator callbacks",
			m_conditionCallbacks.size(), m_commandCallbacks.size(), m_operatorCallbacks.size());

		// Release all condition callbacks
		for (auto& [type, callback] : m_conditionCallbacks)
		{
			wrenReleaseHandle(vm, callback);
		}

		// Release all command callbacks
		for (auto& [type, callback] : m_commandCallbacks)
		{
			wrenReleaseHandle(vm, callback);
		}

		// Release all operator callbacks
		for (auto& [op, callback] : m_operatorCallbacks)
		{
			wrenReleaseHandle(vm, callback);
		}
    }

    void DialogueRegistry::RegisterConditionType(GameContext& context, const std::string& type, WrenHandle* callback)
	{
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		// If type already exists, release old handle
		auto it = m_conditionCallbacks.find(type);
		if (it != m_conditionCallbacks.end())
		{
			DEBUG_WARNING("Condition type '%s' already registered, replacing", type.c_str());
			wrenReleaseHandle(vm, it->second);
		}

		m_conditionCallbacks[type] = callback;
		DEBUG_INFO("Registered condition type: %s", type.c_str());
	}

	void DialogueRegistry::RegisterCommandType(GameContext& context, const std::string& type, WrenHandle* callback)
	{
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		// If type already exists, release old handle
		auto it = m_commandCallbacks.find(type);
		if (it != m_commandCallbacks.end())
		{
			DEBUG_WARNING("Command type '%s' already registered, replacing", type.c_str());
			wrenReleaseHandle(vm, it->second);
		}

		m_commandCallbacks[type] = callback;
		DEBUG_INFO("Registered command type: %s", type.c_str());
	}

	void DialogueRegistry::RegisterOperator(GameContext& context, const std::string& op, WrenHandle* callback)
	{
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		// If operator already exists, release old handle
		auto it = m_operatorCallbacks.find(op);
		if (it != m_operatorCallbacks.end())
		{
			DEBUG_WARNING("Operator '%s' already registered, replacing", op.c_str());
			wrenReleaseHandle(vm, it->second);
		}

		m_operatorCallbacks[op] = callback;
		DEBUG_INFO("Registered operator: %s", op.c_str());
	}

	std::unique_ptr<Condition> DialogueRegistry::CreateCondition(GameContext& context, const std::string& type, const std::map<std::string, DialogueValue>& params)
	{
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		auto it = m_conditionCallbacks.find(type);
		if (it == m_conditionCallbacks.end())
		{
			DEBUG_ERROR("Condition type '%s' not registered", type.c_str());
			return nullptr;
		}

		return std::make_unique<WrenCondition>(vm, it->second, params);
	}

	std::unique_ptr<Command> DialogueRegistry::CreateCommand(GameContext& context, const std::string& type, const std::map<std::string, DialogueValue>& params)
	{
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		auto it = m_commandCallbacks.find(type);
		if (it == m_commandCallbacks.end())
		{
			DEBUG_ERROR("Command type '%s' not registered", type.c_str());
			return nullptr;
		}

		return std::make_unique<WrenCommand>(vm, it->second, params);
	}

	bool DialogueRegistry::EvaluateOperator(GameContext& context, const std::string& op, const DialogueValue& lhs, const DialogueValue& rhs)
	{
		WrenVM* vm = context.GetWrenScriptEngine().GetVM();
		auto it = m_operatorCallbacks.find(op);
		if (it == m_operatorCallbacks.end())
		{
			DEBUG_ERROR("Operator '%s' not registered", op.c_str());
			return false;
		}

		wrenEnsureSlots(vm, 3);
		wrenSetSlotHandle(vm, 0, it->second);

		// Set lhs in slot 1
		switch (lhs.type)
		{
			case DialogueValue::Type::STRING:
				wrenSetSlotString(vm, 1, lhs.stringValue.c_str());
				break;
			case DialogueValue::Type::INT:
				wrenSetSlotDouble(vm, 1, static_cast<double>(lhs.intValue));
				break;
			case DialogueValue::Type::BOOL:
				wrenSetSlotBool(vm, 1, lhs.boolValue);
				break;
			case DialogueValue::Type::DOUBLE:
				wrenSetSlotDouble(vm, 1, lhs.doubleValue);
				break;
		}

		// Set rhs in slot 2
		switch (rhs.type)
		{
			case DialogueValue::Type::STRING:
				wrenSetSlotString(vm, 2, rhs.stringValue.c_str());
				break;
			case DialogueValue::Type::INT:
				wrenSetSlotDouble(vm, 2, static_cast<double>(rhs.intValue));
				break;
			case DialogueValue::Type::BOOL:
				wrenSetSlotBool(vm, 2, rhs.boolValue);
				break;
			case DialogueValue::Type::DOUBLE:
				wrenSetSlotDouble(vm, 2, rhs.doubleValue);
				break;
		}

		// Call: callback.call(lhs, rhs)
		WrenHandle* method = wrenMakeCallHandle(vm, "call(_,_)");
		WrenInterpretResult result = wrenCall(vm, method);
		wrenReleaseHandle(vm, method);

		if (result != WREN_RESULT_SUCCESS)
		{
			DEBUG_ERROR("Operator '%s' callback failed", op.c_str());
			return false;
		}

		return wrenGetSlotBool(vm, 0);
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
}
