#include "Condition.h"

#include "DialogueHelperFunctions.h"
#include "Engine/Callback/Callback.h"
#include "Engine/GameContext.h"

Struktur::Dialogue::Condition::Condition(const std::string& key,
                                         const std::unordered_map<std::string, DialogueValue>& params)
    : m_key(key),
      m_params(params)
{
}

bool Struktur::Dialogue::Condition::Evaluate(GameContext& context) const
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

Struktur::Callback::ICallback* Struktur::Dialogue::Condition::GetCallback(GameContext& context) const
{
	DialogueRegistry& dialogueRegistry = context.GetDialogueRegistry();
	return dialogueRegistry.GetCondition(m_key);
}

const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& Struktur::Dialogue::Condition::GetParams()
    const
{
	return m_params;
}

void Struktur::Dialogue::Condition::SetParameter(const std::string& key, const DialogueValue& value)
{
	m_params[key] = value;
}

void Struktur::Dialogue::Condition::RemoveParameter(const std::string& key)
{
	m_params.erase(key);
}

void Struktur::Dialogue::Condition::ChangeParameterKey(const std::string& oldKey, const std::string& newKey)
{
	auto it = m_params.find(oldKey);
	if (it != m_params.end())
	{
		m_params.emplace(newKey, std::move(it->second));  // move the value to avoid copying
		m_params.erase(it);
	}
}
