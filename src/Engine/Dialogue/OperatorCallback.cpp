#include "OperatorCallback.h"

#include "Engine/GameContext.h"

Struktur::Dialogue::OperatorCondition::OperatorCondition(std::string key, const DialogueValue& lhs, const DialogueValue& rhs)
    : m_key(key), m_lhs(lhs), m_rhs(rhs)
{
}

bool Struktur::Dialogue::OperatorCondition::Evaluate(GameContext &context) const
{
    DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
    auto callback = dialogueRegistry.GetOperator(m_key);
    return callback(m_lhs, m_rhs);
}
