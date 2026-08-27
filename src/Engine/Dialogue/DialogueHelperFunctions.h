#pragma once

#include <unordered_map>
#include <vector>

#include "DialogueNode.h"
#include "DialogueValue.h"
#include "Engine/Callback/Variant.h"
#include "wren.hpp"

namespace Struktur::Dialogue::HelperFunctions
{
DialogueValue ConvertVariantToDialogueValue(Callback::Variant variant);
std::vector<Callback::Variant> ConvertParamsToVariant(std::unordered_map<std::string, DialogueValue> params);

std::vector<std::unique_ptr<DialogueNode>> GetNodeListFromWren(WrenVM* vm, int slot);
std::pair<std::string, std::unique_ptr<DialogueNode>> DialogueParseWrenNodeKeyPair(WrenVM* vm, int slot);
std::unique_ptr<DialogueNode> DialogueParseWrenNodeData(WrenVM* vm, int slot, std::string id);
std::unique_ptr<ConditionalTarget> DialogueParseWrenConditionalTarget(WrenVM* vm, int slot);
std::unique_ptr<Choice> DialogueParseWrenChoice(WrenVM* vm, int slot);
std::unique_ptr<Condition> DialogueParseWrenCondition(WrenVM* vm, int slot);
std::unique_ptr<Command> DialogueParseWrenCommand(WrenVM* vm, int slot);
std::unordered_map<std::string, DialogueValue> DialogueParseWrenParameters(WrenVM* vm, int slot);
std::pair<std::string, DialogueValue> DialogueParseWrenParameter(WrenVM* vm, int slot);
DialogueValue DialogueParseWrenDialogueValue(WrenVM* vm, int slot);
void ConvertParamsMapToWrenMap(WrenVM* vm, int slot, const std::unordered_map<std::string, DialogueValue>& params);
void AddDialogueValueToWren(WrenVM* vm, int slot, const DialogueValue& dialogueValue);
}  // namespace Struktur::Dialogue::HelperFunctions
