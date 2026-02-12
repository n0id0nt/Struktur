#include "CallbackCommand.h"

#include "Engine/GameContext.h"
#include "Engine/Callback/Callback.h"
#include "DialogueHelperFunctions.h"

Struktur::Dialogue::CallbackCommand::CallbackCommand(std::string key, const std::unordered_map<std::string, DialogueValue>& params)
	: m_key(key), m_params(params)
{
}

void Struktur::Dialogue::CallbackCommand::Execute(GameContext& context) const
{
    auto* callback = GetCallback(context);
    
    auto params = HelperFunctions::ConvertParamsToVariant(m_params);
    callback->Invoke(context, params);
}

Struktur::Callback::ICallback *Struktur::Dialogue::CallbackCommand::GetCallback(GameContext &context) const
{
    DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
    return dialogueRegistry.GetCondition(m_key);
}

const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& Struktur::Dialogue::CallbackCommand::GetParams() const
{
    return m_params;
}
