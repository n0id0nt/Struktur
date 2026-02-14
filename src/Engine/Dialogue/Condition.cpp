#include "Condition.h"

#include "Engine/GameContext.h"
#include "DialogueHelperFunctions.h"
#include "Engine/Callback/Callback.h"

Struktur::Dialogue::Condition::Condition(std::string key, const std::unordered_map<std::string, DialogueValue>& params)
    : m_key(key), m_params(params)
{
}

bool Struktur::Dialogue::Condition::Evaluate(GameContext &context) const
{
    auto* callback = GetCallback(context);

    if (!callback)
    {
        BREAK_MSG("[Dialogue] No callback called %s is contained in registry", m_key);
        return false;
    }

    auto params = HelperFunctions::ConvertParamsToVariant(m_params);
    auto result = callback->Invoke(context, params);
    return std::get<bool>(result);
}

Struktur::Callback::ICallback *Struktur::Dialogue::Condition::GetCallback(GameContext &context) const
{
    DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
    return dialogueRegistry.GetCondition(m_key);
}

const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& Struktur::Dialogue::Condition::GetParams() const
{
    return m_params;
}
