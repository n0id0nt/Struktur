#include "CallbackCondition.h"

#include "Engine/GameContext.h"

Struktur::Dialogue::CallbackCondition::CallbackCondition(std::string key, const std::map<std::string, DialogueValue>& params)
    : m_key(key), m_params(params)
{
}

bool Struktur::Dialogue::CallbackCondition::Evaluate(GameContext &context) const
{
    DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
    auto callback = dialogueRegistry.GetCondition(m_key);
    return callback(m_params);
}
