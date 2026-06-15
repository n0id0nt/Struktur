#include "DialogueHelperFunctions.h"

namespace Struktur::Dialogue::HelperFunctions
{
DialogueValue ConvertVariantToDialogueValue(Callback::Variant variant)
{
	DialogueValue value;
	std::visit(
	    [&value](auto&& arg)
	    {
		    using T = std::decay_t<decltype(arg)>;

		    if constexpr (std::is_same_v<T, std::string>)
		    {
			    value = DialogueValue(arg.c_str());
		    }
		    else if constexpr (std::is_same_v<T, int>)
		    {
			    value = DialogueValue(static_cast<double>(arg));
		    }
		    else if constexpr (std::is_same_v<T, bool>)
		    {
			    value = DialogueValue(arg);
		    }
		    else if constexpr (std::is_same_v<T, double>)
		    {
			    value = DialogueValue(arg);
		    }
	    },
	    variant);
	return value;
}

std::vector<Callback::Variant> ConvertParamsToVariant(std::unordered_map<std::string, DialogueValue> params)
{
	Callback::VariantMap variantMap;

	for (const auto& [key, value] : params)
	{
		Callback::Variant variantValue;

		if (value.IsString())
		{
			variantValue = value.AsString();
		}
		else if (value.IsInt())
		{
			variantValue = value.AsInt();
		}
		else if (value.IsBool())
		{
			variantValue = value.AsBool();
		}
		else if (value.IsDouble())
		{
			variantValue = value.AsDouble();
		}
		else
		{
			variantValue = nullptr;
		}

		variantMap.items.insert({key, variantValue});
	}

	return {variantMap};
}

void AddDialogueValueToWren(WrenVM* vm, int slot, const DialogueValue& dialogueValue)
{
	std::visit(
	    [vm, slot](auto&& arg)
	    {
		    using T = std::decay_t<decltype(arg)>;

		    if constexpr (std::is_same_v<T, std::string>)
		    {
			    wrenSetSlotString(vm, slot, arg.c_str());
		    }
		    else if constexpr (std::is_same_v<T, int>)
		    {
			    wrenSetSlotDouble(vm, slot, static_cast<double>(arg));
		    }
		    else if constexpr (std::is_same_v<T, bool>)
		    {
			    wrenSetSlotBool(vm, slot, arg);
		    }
		    else if constexpr (std::is_same_v<T, double>)
		    {
			    wrenSetSlotDouble(vm, slot, arg);
		    }
		    else if constexpr (std::is_same_v<T, std::shared_ptr<Callback::VariantList>>)
		    {
			    if (!arg)
			    {
				    wrenSetSlotNull(vm, slot);
				    return;
			    }

			    wrenEnsureSlots(vm, slot + 2);
			    wrenSetSlotNewList(vm, slot);

			    // Use slot + 1 as scratch space for each element
			    int elementSlot = slot + 1;
			    for (const auto& item : arg->items)
			    {
				    VariantToWrenSlot(vm, elementSlot, item);
				    wrenInsertInList(vm, slot, -1, elementSlot);
			    }
		    }
		    else if constexpr (std::is_same_v<T, std::shared_ptr<Callback::VariantMap>>)
		    {
			    if (!arg)
			    {
				    wrenSetSlotNull(vm, slot);
				    return;
			    }

			    wrenEnsureSlots(vm, slot + 3);
			    wrenSetSlotNewMap(vm, slot);

			    // Use slot + 1 and slot + 2 as scratch space for key and value
			    int keySlot   = slot + 1;
			    int valueSlot = slot + 2;
			    for (const auto& [key, value] : arg->items)
			    {
				    wrenSetSlotString(vm, keySlot, key.c_str());
				    VariantToWrenSlot(vm, valueSlot, value);
				    wrenSetMapValue(vm, slot, keySlot, valueSlot);
			    }
		    }
		    else
		    {
			    wrenSetSlotNull(vm, slot);
		    }
	    },
	    dialogueValue.GetVariant());
}

void ConvertParamsMapToWrenMap(WrenVM* vm, int slot, const std::unordered_map<std::string, DialogueValue>& params)
{
	// Create a new Wren map in the specified slot
	wrenEnsureSlots(vm, slot + 3);  // Need extra slots for key, value, and map operations
	wrenSetSlotNewMap(vm, slot);

	// Iterate through the params map
	for (const auto& [key, value] : params)
	{
		// Set the key (always a string) in slot+1
		wrenSetSlotString(vm, slot + 1, key.c_str());

		AddDialogueValueToWren(vm, slot + 2, value);

		// Insert the key-value pair into the map
		wrenSetMapValue(vm, slot, slot + 1, slot + 2);
	}
}

DialogueValue DialogueParseWrenDialogueValue(WrenVM* vm, int slot)
{
	switch (wrenGetSlotType(vm, slot))
	{
		case WrenType::WREN_TYPE_BOOL:
			return DialogueValue(wrenGetSlotBool(vm, slot));
		case WrenType::WREN_TYPE_NUM:
			return DialogueValue(wrenGetSlotDouble(vm, slot));
		case WrenType::WREN_TYPE_STRING:
			return DialogueValue(wrenGetSlotString(vm, slot));
	}
	return {};
}

std::pair<std::string, DialogueValue> DialogueParseWrenParameter(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "value");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	DialogueValue value = DialogueParseWrenDialogueValue(vm, slot + 2);

	return {type, value};
}

std::unordered_map<std::string, DialogueValue> DialogueParseWrenParameters(WrenVM* vm, int slot)
{
	std::unordered_map<std::string, DialogueValue> map;
	wrenEnsureSlots(vm, slot + 2);
	int count = wrenGetListCount(vm, slot);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, slot, i, slot + 1);
		auto dialoguePair       = DialogueParseWrenParameter(vm, slot + 1);
		map[dialoguePair.first] = dialoguePair.second;
	}
	return map;
}

std::unique_ptr<Command> DialogueParseWrenCommand(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return std::make_unique<Command>(type, params);
}

std::unique_ptr<Condition> DialogueParseWrenCondition(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return std::make_unique<Condition>(type, params);
}

std::unique_ptr<Choice> DialogueParseWrenChoice(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "text");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string text = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "target");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string target = wrenGetSlotString(vm, slot + 2);

	return std::make_unique<Choice>(text, target);
}

std::unique_ptr<ConditionalTarget> DialogueParseWrenConditionalTarget(WrenVM* vm, int slot)
{
	auto conditionalTarget = std::make_unique<ConditionalTarget>();

	wrenEnsureSlots(vm, slot + 4);
	wrenSetSlotString(vm, slot + 1, "node");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	conditionalTarget->targetNode = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "conditions");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<Condition> target = DialogueParseWrenCondition(vm, slot + 3);

			conditionalTarget->conditions.push_back(std::move(target));
		}
	}

	return conditionalTarget;
}

std::unique_ptr<DialogueNode> DialogueParseWrenNodeData(WrenVM* vm, int slot, std::string id)
{
	wrenEnsureSlots(vm, slot + 4);
	auto node = std::make_unique<DialogueNode>(id);

	wrenSetSlotString(vm, slot + 1, "speaker");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string speaker = wrenGetSlotString(vm, slot + 2);
		node->SetSpeaker(speaker);
	}

	wrenSetSlotString(vm, slot + 1, "text");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string text = wrenGetSlotString(vm, slot + 2);
		node->SetText(text);
	}

	wrenSetSlotString(vm, slot + 1, "commands");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<Command> command = DialogueParseWrenCommand(vm, slot + 3);
			node->AddCommand(std::move(command));
		}
	}

	wrenSetSlotString(vm, slot + 1, "choices");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<Choice> choice = DialogueParseWrenChoice(vm, slot + 3);
			node->AddChoice(std::move(choice));
		}
	}

	wrenSetSlotString(vm, slot + 1, "next");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string next = wrenGetSlotString(vm, slot + 2);
		node->SetNext(next);
	}

	wrenSetSlotString(vm, slot + 1, "targets");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<ConditionalTarget> target = DialogueParseWrenConditionalTarget(vm, slot + 3);
			node->AddTarget(std::move(target));
		}
	}

	return node;
}

std::pair<std::string, std::unique_ptr<DialogueNode>> DialogueParseWrenNodeKeyPair(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);

	wrenSetSlotString(vm, slot + 1, "node");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string nodeKey = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "data");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::unique_ptr<DialogueNode> node = DialogueParseWrenNodeData(vm, slot + 2, nodeKey);

	return {nodeKey, std::move(node)};
}

std::vector<std::unique_ptr<DialogueNode>> GetNodeListFromWren(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, 3);

	int count = wrenGetListCount(vm, 1);
	std::vector<std::unique_ptr<DialogueNode>> nodeList;
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		auto node = DialogueParseWrenNodeKeyPair(vm, 2);

		nodeList.push_back(std::move(node.second));
	}

	return nodeList;
}
}  // namespace Struktur::Dialogue::HelperFunctions
