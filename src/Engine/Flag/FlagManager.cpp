#include "FlagManager.h"

namespace Struktur::Flag
{
	FlagManager::FlagManager()
		: m_boolFlags()
		, m_intFlags()
		, m_floatFlags()
		, m_stringFlags()
	{
	}

	FlagManager::~FlagManager()
	{
	}

	// Boolean flags
	bool FlagManager::GetFlag(const std::string& name) const
	{
		auto it = m_boolFlags.find(name);
		return it != m_boolFlags.end() ? it->second : false;
	}

	void FlagManager::SetFlag(const std::string& name, bool value)
	{
		m_boolFlags[name] = value;
	}

	bool FlagManager::HasFlag(const std::string& name) const
	{
		return m_boolFlags.find(name) != m_boolFlags.end();
	}

	void FlagManager::RemoveFlag(const std::string& name)
	{
		m_boolFlags.erase(name);
	}

	// Integer flags
	int FlagManager::GetIntFlag(const std::string& name) const
	{
		auto it = m_intFlags.find(name);
		return it != m_intFlags.end() ? it->second : 0;
	}

	void FlagManager::SetIntFlag(const std::string& name, int value)
	{
		m_intFlags[name] = value;
	}

	bool FlagManager::HasIntFlag(const std::string& name) const
	{
		return m_intFlags.find(name) != m_intFlags.end();
	}

	void FlagManager::RemoveIntFlag(const std::string& name)
	{
		m_intFlags.erase(name);
	}

	// Float flags
	float FlagManager::GetFloatFlag(const std::string& name) const
	{
		auto it = m_floatFlags.find(name);
		return it != m_floatFlags.end() ? it->second : 0.0f;
	}

	void FlagManager::SetFloatFlag(const std::string& name, float value)
	{
		m_floatFlags[name] = value;
	}

	bool FlagManager::HasFloatFlag(const std::string& name) const
	{
		return m_floatFlags.find(name) != m_floatFlags.end();
	}

	void FlagManager::RemoveFloatFlag(const std::string& name)
	{
		m_floatFlags.erase(name);
	}

	// String flags
	std::string FlagManager::GetStringFlag(const std::string& name) const
	{
		auto it = m_stringFlags.find(name);
		return it != m_stringFlags.end() ? it->second : "";
	}

	void FlagManager::SetStringFlag(const std::string& name, const std::string& value)
	{
		m_stringFlags[name] = value;
	}

	bool FlagManager::HasStringFlag(const std::string& name) const
	{
		return m_stringFlags.find(name) != m_stringFlags.end();
	}

	void FlagManager::RemoveStringFlag(const std::string& name)
	{
		m_stringFlags.erase(name);
	}

	// Utility
	void FlagManager::Clear()
	{
		m_boolFlags.clear();
		m_intFlags.clear();
		m_floatFlags.clear();
		m_stringFlags.clear();
	}

	void FlagManager::ClearBoolFlags()
	{
		m_boolFlags.clear();
	}

	void FlagManager::ClearIntFlags()
	{
		m_intFlags.clear();
	}

	void FlagManager::ClearFloatFlags()
	{
		m_floatFlags.clear();
	}

	void FlagManager::ClearStringFlags()
	{
		m_stringFlags.clear();
	}
}
