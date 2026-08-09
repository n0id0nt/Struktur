#include "LogWindow.h"

#include <raylib.h>

#include <cstdarg>
#include <cstring>
#include <ctime>
#include <sstream>

#include "Engine/GameContext.h"

namespace Struktur::Debug
{
LogWindow* LogWindow::s_instance = nullptr;

LogWindow::LogWindow()
    : EditorWindow("Console"),
      m_showTrace(true),
      m_showDebug(true),
      m_showInfo(true),
      m_showWarning(true),
      m_showError(true),
      m_showFatal(true),
      m_autoScroll(true),
      m_scrollToBottom(false),
      m_selectedLogIndex(-1)
{
	m_searchBuffer[0] = '\0';
	s_instance        = this;

#if defined(PLATFORM_WEB)
	// Set Raylib's trace log callback to our function. Desktop doesn't do this: raylib's window/core state is
	// never initialised there (bgfx replaces it), and registering a custom trace callback against that
	// never-initialised state hangs the process - DEBUG_INFO/etc (which route through raylib's ::TraceLog,
	// see Debug/Assertions.h) keep working either way, this window just can't capture them into its own view.
	SetTraceLogCallback(TraceLogCallback);
#endif
}

LogWindow::~LogWindow()
{
#if defined(PLATFORM_WEB)
	// Restore default Raylib logging
	SetTraceLogCallback(nullptr);
#endif
	s_instance = nullptr;
}

void LogWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	// Toolbar with filters and controls
	RenderToolbar();

	ImGui::Separator();

	// Main log display
	RenderLogList();

	ImGui::End();
}

void LogWindow::TraceLogCallback(int logLevel, const char* text, va_list args)
{
	if (!s_instance)
	{
		return;
	}

	// Format the message
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), text, args);

	// Get timestamp
	time_t now = time(nullptr);
	char timeBuffer[64];
	strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", localtime(&now));

	// Add to log
	s_instance->AddLog(logLevel, buffer);
}

void LogWindow::AddLog(int logLevel, const std::string& message)
{
	// Get timestamp
	time_t now = time(nullptr);
	char timeBuffer[64];
	strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", localtime(&now));

	m_logs.emplace_back(logLevel, message, timeBuffer);

	// Limit log size to prevent memory issues (keep last 10000 entries)
	if (m_logs.size() > 10000)
	{
		m_logs.erase(m_logs.begin(), m_logs.begin() + 1000);
	}

	if (m_autoScroll)
	{
		m_scrollToBottom = true;
	}
}

void LogWindow::ClearLogs()
{
	m_logs.clear();
	m_selectedLogIndex = -1;
}

void LogWindow::RenderToolbar()
{
	// Clear button
	if (ImGui::Button("Clear"))
	{
		ClearLogs();
	}
	ImGui::SameLine();

	// Auto-scroll toggle
	ImGui::Checkbox("Auto-scroll", &m_autoScroll);
	ImGui::SameLine();

	// Log count
	ImGui::Text("Logs: %zu", m_logs.size());
	ImGui::SameLine();

	// Search box
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputTextWithHint("##search", "Search...", m_searchBuffer, sizeof(m_searchBuffer));

	ImGui::Separator();

	// Filter checkboxes
	ImGui::Text("Filters:");
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LOG_TRACE));
	ImGui::Checkbox("Trace", &m_showTrace);
	ImGui::PopStyleColor();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LOG_DEBUG));
	ImGui::Checkbox("Debug", &m_showDebug);
	ImGui::PopStyleColor();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LOG_INFO));
	ImGui::Checkbox("Info", &m_showInfo);
	ImGui::PopStyleColor();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LOG_WARNING));
	ImGui::Checkbox("Warning", &m_showWarning);
	ImGui::PopStyleColor();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LOG_ERROR));
	ImGui::Checkbox("Error", &m_showError);
	ImGui::PopStyleColor();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LOG_FATAL));
	ImGui::Checkbox("Fatal", &m_showFatal);
	ImGui::PopStyleColor();
}

void LogWindow::RenderLogList()
{
	// Create a child window for the log list with scrolling
	ImGui::BeginChild("LogListRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

	// Build filtered list first to avoid clipper issues
	std::vector<int> filteredIndices;
	for (int i = 0; i < (int)m_logs.size(); i++)
	{
		const LogEntry& entry = m_logs[i];

		// Apply filters
		if (!ShouldShowLog(entry))
		{
			continue;
		}

		// Apply search filter
		if (m_searchBuffer[0] != '\0')
		{
			if (entry.message.find(m_searchBuffer) == std::string::npos)
			{
				continue;
			}
		}

		filteredIndices.push_back(i);
	}

	// Use clipper only if we have items to display
	if (!filteredIndices.empty())
	{
		// Clipper only works cleanly with uniform height rows.
		// For expanded multi-line entries we skip the clipper and render manually.
		// If you have thousands of logs and many expanded, this is a tradeoff worth making.
		bool hasExpanded = !m_expandedLogs.empty();

		if (!hasExpanded)
		{
			ImGuiListClipper clipper;
			clipper.Begin((int)filteredIndices.size());
			while (clipper.Step())
			{
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
				{
					RenderLogRow(filteredIndices[row]);
				}
			}
		}
		else
		{
			// Manual render when expanded rows exist (variable height)
			for (int idx : filteredIndices)
			{
				RenderLogRow(idx);
			}
		}
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No logs to display");
	}

	// Deferred scroll � happens after all content is laid out
	if (m_scrollToBottom)
	{
		ImGui::SetScrollHereY(1.0f);
		m_scrollToBottom = false;
	}

	ImGui::EndChild();
}

void LogWindow::RenderLogRow(int i)
{
	const LogEntry& entry = m_logs[i];
	ImVec4 color          = GetLogLevelColor(entry.logLevel);
	ImVec4 grey           = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
	ImVec4 dimGrey        = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	bool isExpanded       = m_expandedLogs.count(i) > 0;

	ImGui::PushID(i);

	// Check if message has multiple lines
	bool isMultiLine = entry.message.find('\n') != std::string::npos;

	// Extract first line for collapsed display
	std::string firstLine = entry.message;
	if (isMultiLine)
	{
		firstLine = entry.message.substr(0, entry.message.find('\n'));
	}

	// Timestamp
	ImGui::TextColored(dimGrey, "[%s]", entry.timestamp.c_str());
	ImGui::SameLine();

	// Log level
	ImGui::TextColored(color, "[%s]", GetLogLevelName(entry.logLevel));
	ImGui::SameLine();

	if (isMultiLine)
	{
		// Expand/collapse arrow
		ImGui::PushStyleColor(ImGuiCol_Text, grey);
		std::string arrowLabel = (isExpanded ? "v " : "> ") + firstLine + "##sel";
		if (ImGui::Selectable(arrowLabel.c_str(), isExpanded, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (isExpanded)
			{
				m_expandedLogs.erase(i);
			}
			else
			{
				m_expandedLogs.insert(i);
			}

			m_selectedLogIndex = i;

			if (ImGui::IsMouseDoubleClicked(0))
			{
				ImGui::SetClipboardText(entry.message.c_str());
			}
		}
		ImGui::PopStyleColor();

		// Render remaining lines if expanded
		if (isExpanded)
		{
			// Indent to align with message column
			ImGui::Indent(16.0f);

			std::istringstream stream(entry.message);
			std::string line;
			bool firstLine = true;
			while (std::getline(stream, line))
			{
				if (firstLine)
				{
					firstLine = false;
					continue;
				}  // skip first, already shown
				ImGui::TextColored(grey, "%s", line.c_str());
			}

			ImGui::Unindent(16.0f);
		}
	}
	else
	{
		// Single line � simple selectable
		bool isSelected = (m_selectedLogIndex == i);
		ImGui::PushStyleColor(ImGuiCol_Text, grey);
		if (ImGui::Selectable(entry.message.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
		{
			m_selectedLogIndex = i;
			if (ImGui::IsMouseDoubleClicked(0))
			{
				ImGui::SetClipboardText(entry.message.c_str());
			}
		}
		ImGui::PopStyleColor();
	}

	ImGui::PopID();
}

const char* LogWindow::GetLogLevelName(int level) const
{
	switch (level)
	{
		case LOG_TRACE:
			return "TRACE";
		case LOG_DEBUG:
			return "DEBUG";
		case LOG_INFO:
			return "INFO";
		case LOG_WARNING:
			return "WARN";
		case LOG_ERROR:
			return "ERROR";
		case LOG_FATAL:
			return "FATAL";
		default:
			return "UNKNOWN";
	}
}

ImVec4 LogWindow::GetLogLevelColor(int level) const
{
	switch (level)
	{
		case LOG_TRACE:
			return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
		case LOG_DEBUG:
			return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Light Blue
		case LOG_INFO:
			return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);  // Green
		case LOG_WARNING:
			return ImVec4(1.0f, 1.0f, 0.4f, 1.0f);  // Yellow
		case LOG_ERROR:
			return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Red
		case LOG_FATAL:
			return ImVec4(1.0f, 0.0f, 1.0f, 1.0f);  // Magenta
		default:
			return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
	}
}

bool LogWindow::ShouldShowLog(const LogEntry& entry) const
{
	switch (entry.logLevel)
	{
		case LOG_TRACE:
			return m_showTrace;
		case LOG_DEBUG:
			return m_showDebug;
		case LOG_INFO:
			return m_showInfo;
		case LOG_WARNING:
			return m_showWarning;
		case LOG_ERROR:
			return m_showError;
		case LOG_FATAL:
			return m_showFatal;
		default:
			return true;
	}
}
}  // namespace Struktur::Debug
