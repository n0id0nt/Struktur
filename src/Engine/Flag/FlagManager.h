#pragma once

#include <string>
#include <unordered_map>

namespace Struktur::Flag
{
	class FlagManager
	{
	public:
		FlagManager();
		~FlagManager();

		// Boolean flags
		bool GetFlag(const std::string& name) const;
		void SetFlag(const std::string& name, bool value);
		bool HasFlag(const std::string& name) const;
		void RemoveFlag(const std::string& name);

		// Integer flags
		int GetIntFlag(const std::string& name) const;
		void SetIntFlag(const std::string& name, int value);
		bool HasIntFlag(const std::string& name) const;
		void RemoveIntFlag(const std::string& name);

		// Float flags
		float GetFloatFlag(const std::string& name) const;
		void SetFloatFlag(const std::string& name, float value);
		bool HasFloatFlag(const std::string& name) const;
		void RemoveFloatFlag(const std::string& name);

		// String flags
		std::string GetStringFlag(const std::string& name) const;
		void SetStringFlag(const std::string& name, const std::string& value);
		bool HasStringFlag(const std::string& name) const;
		void RemoveStringFlag(const std::string& name);

		// Utility
		void Clear();
		void ClearBoolFlags();
		void ClearIntFlags();
		void ClearFloatFlags();
		void ClearStringFlags();

		// Debug/Inspection
		size_t GetBoolFlagCount() const { return m_boolFlags.size(); }
		size_t GetIntFlagCount() const { return m_intFlags.size(); }
		size_t GetFloatFlagCount() const { return m_floatFlags.size(); }
		size_t GetStringFlagCount() const { return m_stringFlags.size(); }

		const std::unordered_map<std::string, bool>& GetAllBoolFlags() const { return m_boolFlags; }
		const std::unordered_map<std::string, int>& GetAllIntFlags() const { return m_intFlags; }
		const std::unordered_map<std::string, float>& GetAllFloatFlags() const { return m_floatFlags; }
		const std::unordered_map<std::string, std::string>& GetAllStringFlags() const { return m_stringFlags; }

	private:
		std::unordered_map<std::string, bool> m_boolFlags;
		std::unordered_map<std::string, int> m_intFlags;
		std::unordered_map<std::string, float> m_floatFlags;
		std::unordered_map<std::string, std::string> m_stringFlags;
	};
}
