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
            std::string targetDockName; // Which dock to attach to ("main", "left", "right", etc.)
        };
        
        std::vector<DockSplit> splits;
    };
    
    class EditorLayoutManager
    {
    public:
        // Setup a layout configuration
        void SetupLayout(ImGuiID dockspace_id, const std::string& layoutName, ImVec2 viewportSize)
        {
            // Only setup once per layout
            if (m_layoutsSetup.find(layoutName) != m_layoutsSetup.end())
                return;
                
            m_layoutsSetup[layoutName] = true;
            
            // Clear existing layout
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewportSize);
            
            // Get the configuration for this layout
            auto it = m_layouts.find(layoutName);
            if (it != m_layouts.end())
            {
                ApplyLayoutConfig(dockspace_id, it->second);
            }
            else
            {
                // Apply default layout if no config found
                ApplyDefaultLayout(dockspace_id);
            }
            
            ImGui::DockBuilderFinish(dockspace_id);
        }
        
        // Register a layout configuration
        void RegisterLayout(const std::string& name, const LayoutConfig& config)
        {
            m_layouts[name] = config;
        }
        
        // Create a default layout
        void CreateDefaultLayout()
        {
            LayoutConfig defaultLayout;
            defaultLayout.name = "Default";
            
            // Split left panel (20% - Hierarchy)
            defaultLayout.splits.push_back({
                "Hierarchy",
                ImGuiDir_Left,
                0.2f,
                "main"
            });
            
            // Split right panel (30% - Inspector + Preview stacked)
            defaultLayout.splits.push_back({
                "Inspector",
                ImGuiDir_Right,
                0.3f,
                "main"
            });
            
            // Split bottom left for file explorer
            defaultLayout.splits.push_back({
                "File Explorer",
                ImGuiDir_Down,
                0.3f,
                "main"
            });
            
            // Split top toolbar (5%)
            defaultLayout.splits.push_back({
                "Toolbar",
                ImGuiDir_Up,
                0.05f,
                "main"
            });
            
            // Game viewport goes in the remaining center
            defaultLayout.splits.push_back({
                "Game Viewport",
                ImGuiDir_None,
                0.0f,
                "main"
            });
            
            RegisterLayout("Default", defaultLayout);
        }
        
        // Save current layout to ini file
        void SaveLayout(const std::string& filename)
        {
            ImGui::SaveIniSettingsToDisk(filename.c_str());
        }
        
        // Load layout from ini file
        void LoadLayout(const std::string& filename)
        {
            ImGui::LoadIniSettingsFromDisk(filename.c_str());
        }
        
        // Reset to specific layout
        void ResetToLayout(const std::string& layoutName)
        {
            m_layoutsSetup.erase(layoutName);
        }
        
        // Clear all setup flags (force re-layout)
        void ResetAllLayouts()
        {
            m_layoutsSetup.clear();
        }
        
    private:
        void ApplyLayoutConfig(ImGuiID dockspace_id, const LayoutConfig& config)
        {
            std::unordered_map<std::string, ImGuiID> dockIds;
            dockIds["main"] = dockspace_id;
            
            // Process splits
            for (const auto& split : config.splits)
            {
                if (split.direction == ImGuiDir_None)
                {
                    // No split, just dock the window
                    ImGuiID targetDock = dockIds[split.targetDockName];
                    ImGui::DockBuilderDockWindow(split.windowName.c_str(), targetDock);
                }
                else
                {
                    // Create a split
                    ImGuiID targetDock = dockIds[split.targetDockName];
                    ImGuiID newDock = 0;
                    ImGuiID remainingDock = 0;
                    
                    newDock = ImGui::DockBuilderSplitNode(
                        targetDock, 
                        split.direction, 
                        split.ratio, 
                        nullptr, 
                        &remainingDock
                    );
                    
                    // Dock the window to the new split
                    ImGui::DockBuilderDockWindow(split.windowName.c_str(), newDock);
                    
                    // Update the main dock to point to remaining space
                    dockIds["main"] = remainingDock;
                }
            }
        }
        
        void ApplyDefaultLayout(ImGuiID dockspace_id)
        {
            ImGuiID dock_main_id = dockspace_id;
            
            // Split left for hierarchy (20%)
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            
            // Split right for inspector/preview (30%)
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
            
            // Split right panel vertically for Inspector (top) and Preview (bottom)
            ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.5f, nullptr, &dock_right_id);
            
            // Split bottom of main area for File Explorer
            ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);
            
            // Split top for toolbar (5%)
            ImGuiID dock_top_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.05f, nullptr, &dock_main_id);
            
            // Dock windows
            ImGui::DockBuilderDockWindow("Game Viewport", dock_main_id);
            ImGui::DockBuilderDockWindow("Hierarchy", dock_left_id);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
            ImGui::DockBuilderDockWindow("Preview", dock_right_bottom);
            ImGui::DockBuilderDockWindow("File Explorer", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Debug Info", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Toolbar", dock_top_id);
        }
        
    private:
        std::unordered_map<std::string, LayoutConfig> m_layouts;
        std::unordered_map<std::string, bool> m_layoutsSetup;
    };
}
