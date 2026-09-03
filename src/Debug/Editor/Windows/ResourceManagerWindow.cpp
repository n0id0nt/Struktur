#include "ResourceManagerWindow.h"

#include <cstdio>

#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/MusicResource.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Resource/ShaderResource.h"
#include "Engine/Resource/SoundResource.h"
#include "Engine/Resource/TextureResource.h"
#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Debug/Editor/Windows/PreviewWindow.h"

namespace Struktur::Debug
{
namespace
{
std::string FormatBytes(size_t bytes)
{
	static const char* kUnits[] = {"B", "KB", "MB", "GB"};
	double value                = (double)bytes;
	int unit                    = 0;
	while (value >= 1024.0 && unit < 3)
	{
		value /= 1024.0;
		++unit;
	}
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.1f %s", value, kUnits[unit]);
	return buffer;
}

std::string FormatMilliseconds(double seconds)
{
	if (seconds < 0.0)
	{
		return "-";
	}
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.2f ms", seconds * 1000.0);
	return buffer;
}

// Adds a red tint once a GPU pool is nearly full - the raw percent text already shows the real number, this is
// just a glance-able warning on top of it.
void RenderGpuBudgetBar(const char* label, size_t loadedCount, size_t cpuMemory, size_t gpuMemory, size_t maxGpu,
                        float percent)
{
	ImGui::Text("%s: %zu (%s CPU)  |  GPU %s / %s (%.0f%%)", label, loadedCount, FormatBytes(cpuMemory).c_str(),
	           FormatBytes(gpuMemory).c_str(), FormatBytes(maxGpu).c_str(), percent);
	bool warn = percent >= 90.0f;
	if (warn)
	{
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
	}
	ImGui::ProgressBar(percent / 100.0f, ImVec2(-1, 0));
	if (warn)
	{
		ImGui::PopStyleColor();
	}
}
}  // namespace

ResourceManagerWindow::ResourceManagerWindow(PreviewWindow* previewWindow)
    : EditorWindow("Resource Manager"),
      m_previewWindow(previewWindow),
      m_startTime(std::chrono::steady_clock::now())
{
}

void ResourceManagerWindow::Initialise(GameContext& context)
{
	// One registration covers all five pools - see ResourceManager::SetResourceEventCallback's own comment for
	// how it tags each pool's events with the right ResourceCategory before they reach here.
	context.GetResourceManager().SetResourceEventCallback(
	    [this](Resource::ResourceCategory category, Resource::ResourceEventType type, const std::string& name,
	           size_t memoryBytes, double seconds) { OnResourceEvent(category, type, name, memoryBytes, seconds); });
}

std::string ResourceManagerWindow::TimingKey(Resource::ResourceCategory category, const std::string& name)
{
	// Prefixed by category so e.g. a Sound and a Music resource that happen to share a name/path don't collide.
	return std::to_string((int)category) + "|" + name;
}

const char* ResourceManagerWindow::CategoryName(Resource::ResourceCategory category)
{
	switch (category)
	{
	case Resource::ResourceCategory::Texture:
		return "Texture";
	case Resource::ResourceCategory::Sound:
		return "Sound";
	case Resource::ResourceCategory::Music:
		return "Music";
	case Resource::ResourceCategory::Font:
		return "Font";
	case Resource::ResourceCategory::Shader:
		return "Shader";
	}
	return "?";
}

const char* ResourceManagerWindow::EventTypeName(Resource::ResourceEventType type)
{
	switch (type)
	{
	case Resource::ResourceEventType::Loaded:
		return "Loaded";
	case Resource::ResourceEventType::ReadyForUse:
		return "ReadyForUse";
	case Resource::ResourceEventType::Unloaded:
		return "Unloaded";
	}
	return "?";
}

void ResourceManagerWindow::OnResourceEvent(Resource::ResourceCategory category, Resource::ResourceEventType type,
                                            const std::string& name, size_t memoryBytes, double seconds)
{
	std::string key = TimingKey(category, name);
	if (type == Resource::ResourceEventType::Loaded)
	{
		m_timings[key].lastLoadSeconds = seconds;
	}
	else if (type == Resource::ResourceEventType::ReadyForUse)
	{
		m_timings[key].lastReadySeconds = seconds;
	}
	else if (type == Resource::ResourceEventType::Unloaded)
	{
		// Drop stale timing - a later reload of the same name gets fresh numbers from its own Loaded event
		// rather than showing a previous life's timing before that happens.
		m_timings.erase(key);
	}

	double timestamp = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_startTime).count();
	m_eventLog.push_front(EventLogEntry{timestamp, category, type, name, memoryBytes, seconds});
	while (m_eventLog.size() > kMaxEventLogEntries)
	{
		m_eventLog.pop_back();
	}
}

void ResourceManagerWindow::CollectRows(GameContext& context, std::vector<RowData>& outRows) const
{
	Resource::ResourceManager& resourceManager = context.GetResourceManager();

	resourceManager.GetTexturePool().ForEachEntry(
	    [&](const std::string& name, const Resource::TextureResource& resource, size_t refCount, bool pinned)
	    {
		    RowData row;
		    row.category   = Resource::ResourceCategory::Texture;
		    row.name       = name;
		    row.filePath   = resource.filePath;
		    row.refCount   = refCount;
		    row.pinned     = pinned;
		    row.isLoaded   = resource.isLoaded;
		    row.cpuMemory  = resource.GetMemoryUsage();
		    row.hasGpuInfo = true;
		    row.gpuMemory  = resource.GetGpuMemoryUsage();
		    row.gpuState   = resource.gpuState;
		    outRows.push_back(std::move(row));
	    });

	resourceManager.GetFontPool().ForEachEntry(
	    [&](const std::string& name, const Resource::FontResource& resource, size_t refCount, bool pinned)
	    {
		    RowData row;
		    row.category   = Resource::ResourceCategory::Font;
		    row.name       = name;
		    row.filePath   = resource.filePath;
		    row.refCount   = refCount;
		    row.pinned     = pinned;
		    row.isLoaded   = resource.isLoaded;
		    row.cpuMemory  = resource.GetMemoryUsage();
		    row.hasGpuInfo = true;
		    row.gpuMemory  = resource.GetGpuMemoryUsage();
		    row.gpuState   = resource.gpuState;
		    outRows.push_back(std::move(row));
	    });

	resourceManager.GetShaderPool().ForEachEntry(
	    [&](const std::string& name, const Resource::ShaderResource& resource, size_t refCount, bool pinned)
	    {
		    RowData row;
		    row.category   = Resource::ResourceCategory::Shader;
		    row.name       = name;
		    row.filePath   = resource.filePath;
		    row.refCount   = refCount;
		    row.pinned     = pinned;
		    row.isLoaded   = resource.isLoaded;
		    row.cpuMemory  = resource.GetMemoryUsage();
		    row.hasGpuInfo = true;
		    row.gpuMemory  = resource.GetGpuMemoryUsage();
		    row.gpuState   = resource.gpuState;
		    outRows.push_back(std::move(row));
	    });

	resourceManager.GetSoundPool().ForEachEntry(
	    [&](const std::string& name, const Resource::SoundResource& resource, size_t refCount, bool pinned)
	    {
		    RowData row;
		    row.category  = Resource::ResourceCategory::Sound;
		    row.name      = name;
		    row.filePath  = resource.filePath;
		    row.refCount  = refCount;
		    row.pinned    = pinned;
		    row.isLoaded  = resource.isLoaded;
		    row.cpuMemory = resource.GetMemoryUsage();
		    outRows.push_back(std::move(row));
	    });

	resourceManager.GetMusicPool().ForEachEntry(
	    [&](const std::string& name, const Resource::MusicResource& resource, size_t refCount, bool pinned)
	    {
		    RowData row;
		    row.category  = Resource::ResourceCategory::Music;
		    row.name      = name;
		    row.filePath  = resource.filePath;
		    row.refCount  = refCount;
		    row.pinned    = pinned;
		    row.isLoaded  = resource.isLoaded;
		    row.cpuMemory = resource.GetMemoryUsage();
		    outRows.push_back(std::move(row));
	    });
}

void ResourceManagerWindow::RenderToolbar(GameContext& context)
{
	static const char* kCategoryNames[] = {"All", "Texture", "Sound", "Music", "Font", "Shader"};
	int selected                        = m_categoryFilter + 1;  // -1..4 -> 0..5
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::Combo("Category", &selected, kCategoryNames, IM_ARRAYSIZE(kCategoryNames)))
	{
		m_categoryFilter = selected - 1;
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputTextWithHint("##NameFilter", "Filter by name...", m_nameFilter, sizeof(m_nameFilter));

	ImGui::SameLine();
	if (ImGui::Checkbox("Pause", &m_paused))
	{
		// Freezes only the table (see RenderTable) - the summary bar and event log below keep updating live
		// either way, matching the plan's "pause the table, not the whole window" intent.
		if (m_paused)
		{
			m_pausedRows.clear();
			CollectRows(context, m_pausedRows);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Dump to Log"))
	{
		context.GetResourceManager().PrintResourceStats();
	}

	ImGui::Separator();
}

void ResourceManagerWindow::RenderSummary(GameContext& context)
{
	Resource::ResourceManager& resourceManager = context.GetResourceManager();

	auto& texturePool = resourceManager.GetTexturePool();
	RenderGpuBudgetBar("Textures", texturePool.GetLoadedCount(), texturePool.GetTotalMemoryUsage(),
	                   texturePool.GetGpuMemoryUsage(), texturePool.GetMaxGpuMemory(),
	                   texturePool.GetGpuMemoryUsagePercent());

	auto& fontPool = resourceManager.GetFontPool();
	RenderGpuBudgetBar("Fonts", fontPool.GetLoadedCount(), fontPool.GetTotalMemoryUsage(),
	                   fontPool.GetGpuMemoryUsage(), fontPool.GetMaxGpuMemory(), fontPool.GetGpuMemoryUsagePercent());

	auto& shaderPool = resourceManager.GetShaderPool();
	RenderGpuBudgetBar("Shaders", shaderPool.GetLoadedCount(), shaderPool.GetTotalMemoryUsage(),
	                   shaderPool.GetGpuMemoryUsage(), shaderPool.GetMaxGpuMemory(),
	                   shaderPool.GetGpuMemoryUsagePercent());

	auto& soundPool = resourceManager.GetSoundPool();
	ImGui::Text("Sounds: %zu (%s)", soundPool.GetLoadedCount(), FormatBytes(soundPool.GetTotalMemoryUsage()).c_str());

	auto& musicPool = resourceManager.GetMusicPool();
	ImGui::Text("Music: %zu (%s)", musicPool.GetLoadedCount(), FormatBytes(musicPool.GetTotalMemoryUsage()).c_str());

	ImGui::Separator();
}

void ResourceManagerWindow::RenderTable(GameContext& context, const std::vector<RowData>& rows)
{
	if (!ImGui::BeginTable("ResourceTable", 9,
	                       ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
	                           ImGuiTableFlags_ScrollY,
	                       ImVec2(0, 300)))
	{
		return;
	}

	ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 55.0f);
	ImGui::TableSetupColumn("Refs", ImGuiTableColumnFlags_WidthFixed, 40.0f);
	ImGui::TableSetupColumn("Pinned", ImGuiTableColumnFlags_WidthFixed, 50.0f);
	ImGui::TableSetupColumn("CPU Mem", ImGuiTableColumnFlags_WidthFixed, 75.0f);
	ImGui::TableSetupColumn("GPU Mem", ImGuiTableColumnFlags_WidthFixed, 75.0f);
	ImGui::TableSetupColumn("GPU State", ImGuiTableColumnFlags_WidthFixed, 90.0f);
	ImGui::TableSetupColumn("Disk Size", ImGuiTableColumnFlags_WidthFixed, 75.0f);
	ImGui::TableSetupColumn("Load / Upload", ImGuiTableColumnFlags_WidthFixed, 140.0f);
	ImGui::TableHeadersRow();

	for (const RowData& row : rows)
	{
		if (m_categoryFilter >= 0 && (int)row.category != m_categoryFilter)
		{
			continue;
		}
		if (m_nameFilter[0] != '\0' && row.name.find(m_nameFilter) == std::string::npos)
		{
			continue;
		}

		ImGui::PushID(row.name.c_str());
		ImGui::PushID((int)row.category);
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		bool canPreview = row.category == Resource::ResourceCategory::Texture && m_previewWindow;
		if (canPreview)
		{
			if (ImGui::Selectable(row.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
			{
				auto texturePtr = context.GetResourceManager().GetEditorTexture(context, row.name);
				PreviewTexture(m_previewWindow, texturePtr, row.name);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Click to preview");
			}
		}
		else
		{
			ImGui::TextUnformatted(row.name.c_str());
		}

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(CategoryName(row.category));

		ImGui::TableNextColumn();
		ImGui::Text("%zu", row.refCount);

		ImGui::TableNextColumn();
		if (row.pinned)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.0f, 1.0f), "Pinned");
		}

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatBytes(row.cpuMemory).c_str());

		ImGui::TableNextColumn();
		if (row.hasGpuInfo)
		{
			ImGui::TextUnformatted(FormatBytes(row.gpuMemory).c_str());
		}
		else
		{
			ImGui::TextDisabled("-");
		}

		ImGui::TableNextColumn();
		if (row.hasGpuInfo)
		{
			const char* stateText = "Unloaded";
			ImVec4 colour         = ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
			if (row.gpuState == Resource::GpuResource::GpuState::LoadedToGpu)
			{
				stateText = "LoadedToGpu";
				colour    = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
			}
			else if (row.gpuState == Resource::GpuResource::GpuState::GpuLost)
			{
				stateText = "GpuLost";
				colour    = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
			}
			ImGui::TextColored(colour, "%s", stateText);
		}
		else
		{
			ImGui::TextDisabled("-");
		}

		ImGui::TableNextColumn();
		auto sizeResult = FileSystem::GetFileSize(row.filePath);
		if (sizeResult)
		{
			ImGui::TextUnformatted(FormatBytes((size_t)sizeResult.value).c_str());
		}
		else
		{
			ImGui::TextDisabled("N/A");
		}

		ImGui::TableNextColumn();
		auto timingIt = m_timings.find(TimingKey(row.category, row.name));
		if (timingIt != m_timings.end())
		{
			ImGui::Text("%s / %s", FormatMilliseconds(timingIt->second.lastLoadSeconds).c_str(),
			           FormatMilliseconds(timingIt->second.lastReadySeconds).c_str());
		}
		else
		{
			ImGui::TextDisabled("- / -");
		}

		ImGui::PopID();
		ImGui::PopID();
	}

	ImGui::EndTable();
}

void ResourceManagerWindow::RenderEventLog()
{
	ImGui::Text("Event Log (newest first, capped at %zu)", kMaxEventLogEntries);
	if (ImGui::BeginChild("EventLogChild", ImVec2(0, 150), true))
	{
		for (const EventLogEntry& event : m_eventLog)
		{
			ImVec4 colour = ImVec4(0.9f, 0.5f, 0.5f, 1.0f);
			if (event.type == Resource::ResourceEventType::Loaded)
			{
				colour = ImVec4(0.5f, 0.9f, 0.5f, 1.0f);
			}
			else if (event.type == Resource::ResourceEventType::ReadyForUse)
			{
				colour = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);
			}

			std::string durationText =
			    event.type == Resource::ResourceEventType::Unloaded ? std::string("-") : FormatMilliseconds(event.durationSeconds);

			ImGui::TextColored(colour, "[%7.2fs] %-8s %-12s %-40s %s", event.timestampSeconds,
			                   CategoryName(event.category), EventTypeName(event.type), event.name.c_str(),
			                   durationText.c_str());
		}
	}
	ImGui::EndChild();
}

void ResourceManagerWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	RenderToolbar(context);
	RenderSummary(context);

	std::vector<RowData> liveRows;
	if (!m_paused)
	{
		CollectRows(context, liveRows);
	}
	const std::vector<RowData>& rows = m_paused ? m_pausedRows : liveRows;

	ImGui::Text("%zu resources total (%s)", rows.size(), m_paused ? "paused" : "live");
	RenderTable(context, rows);

	ImGui::Separator();
	RenderEventLog();

	ImGui::End();
}
}  // namespace Struktur::Debug
