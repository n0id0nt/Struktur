#include "CallbackCondition.h"

Struktur::Dialogue::CallbackCondition::CallbackCondition(Callback callback, DisposeCallback disposeCallback)
    : m_callback(callback), m_disposeCallback(disposeCallback)
{
}

void Struktur::Dialogue::CallbackCondition::Dispose(GameContext &context)
{
    m_disposeCallback(context);
}

bool Struktur::Dialogue::CallbackCondition::Evaluate(GameContext &context, const std::map<std::string, DialogueValue>& params)
{
    return m_callback(params);
}
