#pragma once

#include <imgui.h>
#include <imgui_internal.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Struktur::Debug
{
struct LayoutConfig
{
	std::string name;

	// Dock split configuration
	struct DockSplit
	{
		std::string windowName;
		ImGuiDir direction;
		float ratio;
		std::string targetDockName;  // Which dock to attach to ("main", "left", "right", etc.)
	};

	std::vector<DockSplit> splits;
};

class EditorLayoutManager
{
   public:
	// Setup a layout configuration
	void SetupLayout(ImGuiID dockspace_id, const std::string& layoutName, ImVec2 viewportSize);

	// Register a layout configuration
	void RegisterLayout(const std::string& name, const LayoutConfig& config);

	// Create a default layout
	void CreateDefaultLayout();

	// Save current layout to ini file
	void SaveLayout(const std::string& filename);

	// Load layout from ini file
	void LoadLayout(const std::string& filename);

	// Reset to specific layout
	void ResetToLayout(const std::string& layoutName);

	// Clear all setup flags (force re-layout)
	void ResetAllLayouts();

   private:
	void ApplyLayoutConfig(ImGuiID dockspace_id, const LayoutConfig& config);
	void ApplyDefaultLayout(ImGuiID dockspace_id);

	std::unordered_map<std::string, LayoutConfig> m_layouts;
	std::unordered_map<std::string, bool> m_layoutsSetup;
};
}  // namespace Struktur::Debug
