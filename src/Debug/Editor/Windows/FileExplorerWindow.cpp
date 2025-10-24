
#include "FileExplorerWindow.h"

#include "PreviewWindow.h"
#include "Engine/GameContext.h"
#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include <algorithm>

namespace Struktur::Debug
{
    void FileExplorerWindow::Render(GameContext& context)
    {
        if (!m_isVisible)
            return;
        
        ImGui::Begin(m_name.c_str(), &m_isOpen);
        
        // Toolbar
        if (ImGui::Button("< Back"))
        {
            std::filesystem::path current(m_currentPath);
            if (current.has_parent_path() && current != m_assetsPath)
            {
                m_currentPath = current.parent_path().string();
                RefreshFileList();
            }
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Refresh"))
        {
            RefreshFileList();
        }
        ImGui::SameLine();
        
        ImGui::Text("Path: %s", m_currentPath.c_str());
        
        ImGui::Separator();
        
        // File list
        RenderFileList(context);
        
        ImGui::End();
    }
    
    void FileExplorerWindow::SetAssetsPath(const std::string& path)
    {
        m_assetsPath = path;
        m_currentPath = path;
        RefreshFileList();
    }
    
    void FileExplorerWindow::RefreshFileList()
    {
        m_files.clear();
        
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(m_currentPath))
            {
                FileEntry fileEntry;
                fileEntry.name = entry.path().filename().string();
                fileEntry.path = entry.path().string();
                fileEntry.isDirectory = entry.is_directory();
                
                if (!fileEntry.isDirectory)
                {
                    fileEntry.fileSize = std::filesystem::file_size(entry.path());
                    fileEntry.extension = GetFileExtension(fileEntry.name);
                }
                else
                {
                    fileEntry.fileSize = 0;
                    fileEntry.extension = "";
                }
                
                m_files.push_back(fileEntry);
            }
            
            // Sort: directories first, then files alphabetically
            std::sort(m_files.begin(), m_files.end(), [](const FileEntry& a, const FileEntry& b)
            {
                if (a.isDirectory != b.isDirectory)
                    return a.isDirectory;
                return a.name < b.name;
            });
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // Handle error silently or log
        }
    }
    
    void FileExplorerWindow::RenderFileList(GameContext& context)
    {
        // Create table for file list
        if (ImGui::BeginTable("FileTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();
            
            for (const auto& file : m_files)
            {
                ImGui::TableNextRow();
                
                // Name column
                ImGui::TableSetColumnIndex(0);
                
                const char* icon = file.isDirectory ? "[DIR]" : "[FILE]";
                bool isSelected = (m_selectedFile == file.path);
                
                ImGui::PushID(file.path.c_str());
                if (ImGui::Selectable(icon, isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    if (file.isDirectory)
                    {
                        // Navigate into directory
                        m_currentPath = file.path;
                        RefreshFileList();
                    }
                    else
                    {
                        // Select file and preview
                        m_selectedFile = file.path;
                        OnFileSelected(file, context);
                    }
                }
                ImGui::PopID();
                
                ImGui::SameLine();
                ImGui::Text("%s", file.name.c_str());
                
                // Type column
                ImGui::TableSetColumnIndex(1);
                if (file.isDirectory)
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Folder");
                }
                else
                {
                    ImGui::Text("%s", file.extension.c_str());
                }
                
                // Size column
                ImGui::TableSetColumnIndex(2);
                if (!file.isDirectory)
                {
                    ImGui::Text("%s", FormatFileSize(file.fileSize).c_str());
                }
            }
            
            ImGui::EndTable();
        }
    }
    
    void FileExplorerWindow::OnFileSelected(const FileEntry& file, GameContext& context)
    {
        if (!m_previewWindow)
            return;
        
        std::string ext = file.extension;
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        // Determine file type and preview accordingly
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
        {
            PreviewUnknownFile(m_previewWindow, file.name);
        }
        else if (ext == ".txt" || ext == ".cpp" || ext == ".h" || ext == ".hpp" || 
                 ext == ".c" || ext == ".cs" || ext == ".lua" || ext == ".py" ||
                 ext == ".json" || ext == ".xml" || ext == ".glsl" || ext == ".frag" || ext == ".vert")
        {
            PreviewTextFile(m_previewWindow, file.path, file.name);
        }
        else
        {
            PreviewUnknownFile(m_previewWindow, file.name);
        }
    }
    
    std::string FileExplorerWindow::GetFileExtension(const std::string& filename)
    {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos && dotPos < filename.length() - 1)
        {
            return filename.substr(dotPos);
        }
        return "";
    }
    
    std::string FileExplorerWindow::FormatFileSize(size_t bytes)
    {
        const char* units[] = { "B", "KB", "MB", "GB" };
        int unitIndex = 0;
        double size = (double)bytes;
        
        while (size >= 1024.0 && unitIndex < 3)
        {
            size /= 1024.0;
            unitIndex++;
        }
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
        return std::string(buffer);
    }
}