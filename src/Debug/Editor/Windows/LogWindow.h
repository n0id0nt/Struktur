#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "EditorWindow.h"

namespace Struktur::Debug
{
// Log entry structure
struct LogEntry
{
	int logLevel;  // TraceLogLevel from raylib
	std::string message;
	std::string timestamp;

	LogEntry(int level, const std::string& msg, const std::string& time)
	    : logLevel(level),
	      message(msg),
	      timestamp(time)
	{
	}
};

class LogWindow : public EditorWindow
{
   public:
	LogWindow();
	~LogWindow();

	void Render(GameContext& context) override;

	// Static callback for Raylib's SetTraceLogCallback
	static void TraceLogCallback(int logLevel, const char* text, va_list args);

	// Get singleton instance for the callback
	static LogWindow* GetInstance()
	{
		return s_instance;
	}

	// Log management
	void ClearLogs();
	void AddLog(int logLevel, const std::string& message);

   private:
	void RenderToolbar();
	void RenderLogList();
	void RenderLogDetails();
	void RenderLogRow(int i);

	const char* GetLogLevelName(int level) const;
	ImVec4 GetLogLevelColor(int level) const;
	bool ShouldShowLog(const LogEntry& entry) const;

   private:
	// Log storage
	std::vector<LogEntry> m_logs;
	std::unordered_set<int> m_expandedLogs;
	bool m_scrollToBottom;

	// Filter settings
	bool m_showTrace;
	bool m_showDebug;
	bool m_showInfo;
	bool m_showWarning;
	bool m_showError;
	bool m_showFatal;

	// UI state
	char m_searchBuffer[256];
	bool m_autoScroll;
	int m_selectedLogIndex;

	// Singleton instance
	static LogWindow* s_instance;
};
}  // namespace Struktur::Debug
