#pragma once

#include <chrono>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "EditorWindow.h"
#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourceEvents.h"

namespace Struktur::Debug
{
class PreviewWindow;

// Live browser over every resource currently cached across Resource::ResourceManager's five pools (Texture/
// Sound/Music/Font/Shader), plus a real-time load/unload event log and load/GPU-upload timing - see the design
// plan (Resource Manager Editor Window) for the full rationale. Entirely editor-only: reads the new #ifdef
// EDITOR-guarded ForEachEntry/SetResourceEventCallback surface added to ResourcePool<T>/GpuResourcePool<T>/
// ResourceManager, never mutates pool state itself (no force-unload - see the plan's explicit non-goals).
class ResourceManagerWindow : public EditorWindow
{
public:
	explicit ResourceManagerWindow(PreviewWindow* previewWindow);

	void Initialise(GameContext& context) override;
	void Render(GameContext& context) override;

private:
	// One row per cached resource, flattened from whichever pool it came from so the table can be one generic
	// loop instead of five near-identical ones. gpuMemory/gpuState/hasGpuInfo are only meaningful for
	// Texture/Font/Shader (the three GPU-backed pools) - Sound/Music leave hasGpuInfo false.
	struct RowData
	{
		Resource::ResourceCategory category;
		std::string name;  // the pool's own cache key (see ResourcePool<T>::GetName's comment on why this can
		                   // differ from the resource's own filePath for Font/Shader)
		std::string filePath;
		size_t refCount  = 0;
		bool pinned      = false;
		bool isLoaded    = false;
		size_t cpuMemory = 0;

		bool hasGpuInfo = false;
		size_t gpuMemory = 0;
		Resource::GpuResource::GpuState gpuState = Resource::GpuResource::GpuState::Unloaded;
	};

	struct EventLogEntry
	{
		double timestampSeconds = 0.0;  // relative to m_startTime, not wall-clock/game time - see constructor
		Resource::ResourceCategory category;
		Resource::ResourceEventType type;
		std::string name;
		size_t memoryBytes = 0;
		double durationSeconds = 0.0;
	};

	// Last observed Loaded/ReadyForUse duration for one resource, keyed by category+name (see TimingKey) - feeds
	// the table's "Last Load"/"Last GPU Upload" columns. Cleared on Unloaded so a stale timing from a previous
	// life of the same name doesn't linger after it's gone.
	struct TimingInfo
	{
		double lastLoadSeconds  = -1.0;  // -1 = never observed
		double lastReadySeconds = -1.0;
	};

	static std::string TimingKey(Resource::ResourceCategory category, const std::string& name);
	static const char* CategoryName(Resource::ResourceCategory category);
	static const char* EventTypeName(Resource::ResourceEventType type);

	// Registered once with ResourceManager::SetResourceEventCallback (see Initialise) - updates m_timings and
	// appends to m_eventLog. Called synchronously from wherever a pool's GetResource/EnsureResourceReady/
	// EvictIfUnused happens to run (always the game thread, same as every other editor-window Render() call).
	void OnResourceEvent(Resource::ResourceCategory category, Resource::ResourceEventType type,
	                     const std::string& name, size_t memoryBytes, double seconds);

	void CollectRows(GameContext& context, std::vector<RowData>& outRows) const;
	void RenderToolbar(GameContext& context);
	void RenderSummary(GameContext& context);
	void RenderTable(GameContext& context, const std::vector<RowData>& rows);
	void RenderEventLog();

	PreviewWindow* m_previewWindow;
	std::chrono::steady_clock::time_point m_startTime;

	// -1 = All categories
	int m_categoryFilter = -1;
	char m_nameFilter[128] = {};
	bool m_paused = false;
	std::vector<RowData> m_pausedRows;  // snapshot captured the moment Pause is toggled on

	static constexpr size_t kMaxEventLogEntries = 200;
	std::deque<EventLogEntry> m_eventLog;
	std::unordered_map<std::string, TimingInfo> m_timings;
};
}  // namespace Struktur::Debug
