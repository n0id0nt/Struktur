#include "EditorSettings.h"

void Struktur::Debug::EditorSettings::SaveToFile(const std::string &filename) {}

void Struktur::Debug::EditorSettings::LoadFromFile(const std::string &filename) {}

void Struktur::Debug::EditorSettings::RegisterOnChangeCallback(const std::string &settingCategory,
                                                                std::function<void()> callback)
{
	m_changeCallbacks[settingCategory] = callback;
}

void Struktur::Debug::EditorSettings::NotifySettingChanged(const std::string &settingCategory)
{
	auto it = m_changeCallbacks.find(settingCategory);
	if (it != m_changeCallbacks.end())
	{
		it->second();
	}
}
