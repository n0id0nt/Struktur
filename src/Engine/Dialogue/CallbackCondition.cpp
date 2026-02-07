#include "CallbackCondition.h"

#include "Engine/GameContext.h"

Struktur::Dialogue::CallbackCondition::CallbackCondition(std::string key, const std::unordered_map<std::string, DialogueValue>& params)
    : m_key(key), m_params(params)
{
}

bool Struktur::Dialogue::CallbackCondition::Evaluate(GameContext &context) const
{
    DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
    auto callback = dialogueRegistry.GetCondition(m_key);

    if (callback)
        return callback(m_params);

    BREAK_MSG("[Dialogue] No callback called %s is contained in registry", m_key);
    return false;
}
