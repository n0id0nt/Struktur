#include "EditorLayoutManager.h"
#include <imgui_internal.h>

void Struktur::Debug::EditorLayoutManager::SetupLayout(ImGuiID dockspace_id, const std::string &layoutName, ImVec2 viewportSize)
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

void Struktur::Debug::EditorLayoutManager::RegisterLayout(const std::string &name, const LayoutConfig &config)
{
    m_layouts[name] = config;
}

void Struktur::Debug::EditorLayoutManager::CreateDefaultLayout()
{
    LayoutConfig defaultLayout;
    defaultLayout.name = "Default";
    
    // Split left panel (20% - Hierarchy)
    defaultLayout.splits.push_back({
        "UI Hierarchy",
        ImGuiDir_Left,
        0.2f,
        "main"
    });

    // Split left panel (20% - Hierarchy)
    defaultLayout.splits.push_back({
        "Hierarchy",
        ImGuiDir_None,
        0.0f,
        "UI Hierarchy"
    });
    
    // Split right panel (30% - Inspector + Preview stacked)
    defaultLayout.splits.push_back({
        "Inspector",
        ImGuiDir_Right,
        0.3f,
        "main"
    });
    
    // Split right panel (30% - Inspector + Preview stacked)
    defaultLayout.splits.push_back({
        "Dialogue Editor",
        ImGuiDir_None,
        0.0f,
        "Inspector"
    });
    
    defaultLayout.splits.push_back({
        "Preview",
        ImGuiDir_Down,
        0.35f,
        "Inspector"
    });
    
    // Split bottom left for file explorer
    defaultLayout.splits.push_back({
        "File Explorer",
        ImGuiDir_Down,
        0.3f,
        "main"
    });
    
    // Doc console with file explore
    defaultLayout.splits.push_back({
        "Console",
        ImGuiDir_None,
        0.0f,
        "File Explorer"
    });

    // Split right panel (30% - Inspector + Preview stacked)
    defaultLayout.splits.push_back({
        "Profiler",
        ImGuiDir_None,
        0.0f,
        "File Explorer"
    });

    // Split top toolbar (20%)
    defaultLayout.splits.push_back({
        "Toolbar",
        ImGuiDir_Up,
        0.2f,
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

void Struktur::Debug::EditorLayoutManager::SaveLayout(const std::string &filename)
{
    ImGui::SaveIniSettingsToDisk(filename.c_str());
}

void Struktur::Debug::EditorLayoutManager::LoadLayout(const std::string &filename)
{
    ImGui::LoadIniSettingsFromDisk(filename.c_str());
}

void Struktur::Debug::EditorLayoutManager::ResetToLayout(const std::string &layoutName)
{
    m_layoutsSetup.erase(layoutName);
}

void Struktur::Debug::EditorLayoutManager::ResetAllLayouts()
{
    m_layoutsSetup.clear();
}

void Struktur::Debug::EditorLayoutManager::ApplyLayoutConfig(ImGuiID dockspace_id, const LayoutConfig &config)
{
    std::unordered_map<std::string, ImGuiID> dockIds;
    dockIds["main"] = dockspace_id;
    
    auto getTargetDoc = [&dockIds] (const std::string& targetDockName) -> ImGuiID {
        auto it = dockIds.find(targetDockName);
        if (it != dockIds.end())
        {
            return it->second;
        }
        else
        {
            ImGuiID targetDock = ImGui::GetID(targetDockName.c_str());
            dockIds[targetDockName] = targetDock; // Cache it for future use
            return targetDock;
        }
    };

    // Process splits
    for (const auto& split : config.splits)
    {
        if (split.direction == ImGuiDir_None)
        {
            // No split, just dock the window
            ImGuiID targetDock = getTargetDoc(split.targetDockName);
            ImGui::DockBuilderDockWindow(split.windowName.c_str(), targetDock);
        }
        else
        {
            // Create a split
            ImGuiID targetDock = getTargetDoc(split.targetDockName);
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
            dockIds[split.windowName] = newDock;
            dockIds[split.targetDockName] = remainingDock;
        }
    }
}

void Struktur::Debug::EditorLayoutManager::ApplyDefaultLayout(ImGuiID dockspace_id)
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
    ImGui::DockBuilderDockWindow("Console", dock_bottom_id);  
    ImGui::DockBuilderDockWindow("Debug Info", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Toolbar", dock_top_id);
}
