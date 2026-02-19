#include "Command.h"

#include "Engine/GameContext.h"
#include "Engine/Callback/Callback.h"
#include "DialogueHelperFunctions.h"

Struktur::Dialogue::Command::Command(const std::string& key, const std::unordered_map<std::string, DialogueValue>& params)
	: m_key(key), m_params(params)
{
}

void Struktur::Dialogue::Command::Execute(GameContext& context) const
{
	auto* callback = GetCallback(context);

	auto params = HelperFunctions::ConvertParamsToVariant(m_params);
	callback->Invoke(context, params);
}

Struktur::Callback::ICallback* Struktur::Dialogue::Command::GetCallback(GameContext& context) const
{
	DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
	return dialogueRegistry.GetCommand(m_key);
}

const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& Struktur::Dialogue::Command::GetParams() const
{
	return m_params;
}
