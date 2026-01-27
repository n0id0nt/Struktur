#include "WrenCommand.h"

#include "wren.h"
#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
	WrenCommand::WrenCommand(WrenVM* vm, WrenHandle* callback, const std::map<std::string, DialogueValue>& params)
		: m_vm(vm)
		, m_callback(callback)
		, m_parameters(params)
	{
	}

	void WrenCommand::Execute(GameContext& context)
	{
		wrenEnsureSlots(m_vm, 4);
		wrenSetSlotHandle(m_vm, 0, m_callback);

		// Create map with parameters in slot 1
		wrenSetSlotNewMap(m_vm, 1);

		for (const auto& [key, value] : m_parameters)
		{
			// Set key in slot 2
			wrenSetSlotString(m_vm, 2, key.c_str());

			// Set value in slot 3 based on type
			switch (value.type)
			{
				case DialogueValue::Type::STRING:
					wrenSetSlotString(m_vm, 3, value.stringValue.c_str());
					break;
				case DialogueValue::Type::INT:
					wrenSetSlotDouble(m_vm, 3, static_cast<double>(value.intValue));
					break;
				case DialogueValue::Type::BOOL:
					wrenSetSlotBool(m_vm, 3, value.boolValue);
					break;
				case DialogueValue::Type::DOUBLE:
					wrenSetSlotDouble(m_vm, 3, value.doubleValue);
					break;
			}

			// Add to map: map[key] = value
			wrenSetMapValue(m_vm, 1, 2, 3);
		}

		// Call: callback.call(params)
		WrenHandle* method = wrenMakeCallHandle(m_vm, "call(_)");
		WrenInterpretResult result = wrenCall(m_vm, method);
		wrenReleaseHandle(m_vm, method);

		if (result != WREN_RESULT_SUCCESS)
		{
			DEBUG_ERROR("Command callback failed");
		}
	}
}