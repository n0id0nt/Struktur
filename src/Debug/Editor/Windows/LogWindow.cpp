#include "LogWindow.h"
#include "Engine/GameContext.h"
#include <raylib.h>
#include <ctime>
#include <cstdarg>
#include <cstring>

namespace Struktur::Debug
{
    LogWindow* LogWindow::s_instance = nullptr;

    LogWindow::LogWindow()
        : EditorWindow("Console")
        , m_showTrace(true)
        , m_showDebug(true)
        , m_showInfo(true)
        , m_showWarning(true)
        , m_showError(true)
        , m_showFatal(true)
        , m_autoScroll(true)
        , m_selectedLogIndex(-1)
    {
        m_searchBuffer[0] = '\0';
        s_instance = this;

        // Set Raylib's trace log callback to our function
        SetTraceLogCallback(TraceLogCallback);
    }

    LogWindow::~LogWindow()
    {
        // Restore default Raylib logging
        SetTraceLogCallback(nullptr);
        s_instance = nullptr;
    }

    void LogWindow::Render(GameContext& context)
    {
        if (!m_isVisible)
            return;

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
            return;

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
        std::lock_guard<std::mutex> lock(m_logMutex);

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
    }

    void LogWindow::ClearLogs()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
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
        std::lock_guard<std::mutex> lock(m_logMutex);

        // Create a child window for the log list with scrolling
        ImGui::BeginChild("LogListRegion", ImVec2(0, 0), true,
            ImGuiWindowFlags_HorizontalScrollbar);

        // Build filtered list first to avoid clipper issues
        std::vector<int> filteredIndices;
        for (int i = 0; i < (int)m_logs.size(); i++)
        {
            const LogEntry& entry = m_logs[i];

            // Apply filters
            if (!ShouldShowLog(entry))
                continue;

            // Apply search filter
            if (m_searchBuffer[0] != '\0')
            {
                if (entry.message.find(m_searchBuffer) == std::string::npos)
                    continue;
            }

            filteredIndices.push_back(i);
        }

        // Use clipper only if we have items to display
        if (!filteredIndices.empty())
        {
            ImGuiListClipper clipper;
            clipper.Begin((int)filteredIndices.size());

            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    int i = filteredIndices[row];
                    const LogEntry& entry = m_logs[i];

                    // Format log line
                    ImGui::PushID(i);

                    bool isSelected = (m_selectedLogIndex == i);
                    ImVec4 color = GetLogLevelColor(entry.logLevel);

                    // Timestamp
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", entry.timestamp.c_str());
                    ImGui::SameLine();

                    // Log level
                    ImGui::TextColored(color, "[%s]", GetLogLevelName(entry.logLevel));
                    ImGui::SameLine();

                    // Message (selectable)
                    if (ImGui::Selectable(entry.message.c_str(), isSelected,
                        ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        m_selectedLogIndex = i;

                        // Copy to clipboard on double-click
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            ImGui::SetClipboardText(entry.message.c_str());
                        }
                    }

                    ImGui::PopID();
                }
            }

            // Auto-scroll to bottom
            if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        else
        {
            // No logs to display
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No logs to display");
        }

        ImGui::EndChild();
    }

    const char* LogWindow::GetLogLevelName(int level) const
    {
        switch (level)
        {
        case LOG_TRACE:   return "TRACE";
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR:   return "ERROR";
        case LOG_FATAL:   return "FATAL";
        default:          return "UNKNOWN";
        }
    }

    ImVec4 LogWindow::GetLogLevelColor(int level) const
    {
        switch (level)
        {
        case LOG_TRACE:   return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
        case LOG_DEBUG:   return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Light Blue
        case LOG_INFO:    return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);  // Green
        case LOG_WARNING: return ImVec4(1.0f, 1.0f, 0.4f, 1.0f);  // Yellow
        case LOG_ERROR:   return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Red
        case LOG_FATAL:   return ImVec4(1.0f, 0.0f, 1.0f, 1.0f);  // Magenta
        default:          return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
        }
    }

    bool LogWindow::ShouldShowLog(const LogEntry& entry) const
    {
        switch (entry.logLevel)
        {
        case LOG_TRACE:   return m_showTrace;
        case LOG_DEBUG:   return m_showDebug;
        case LOG_INFO:    return m_showInfo;
        case LOG_WARNING: return m_showWarning;
        case LOG_ERROR:   return m_showError;
        case LOG_FATAL:   return m_showFatal;
        default:          return true;
        }
    }
}