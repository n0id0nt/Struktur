#include "CallbackCommand.h"

#include "Engine/GameContext.h"

Struktur::Dialogue::CallbackCommand::CallbackCommand(std::string key, const std::map<std::string, DialogueValue>& params)
	: m_key(key), m_params(params)
{
}

void Struktur::Dialogue::CallbackCommand::Execute(GameContext & context) const
{
    DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
    auto callback = dialogueRegistry.GetCommand(m_key);
    callback(m_params);
}
