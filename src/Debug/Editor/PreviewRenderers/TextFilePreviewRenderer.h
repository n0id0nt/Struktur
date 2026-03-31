#pragma once

#include "PreviewRenderer.h"
#include <string>
#include <fstream>
#include <sstream>
#include "physfs.h"

namespace Struktur::Debug
{
    class TextFilePreviewRenderer : public IPreviewRenderer
    {
    public:
        TextFilePreviewRenderer(const std::string& filePath, const std::string& name)
            : m_filePath(filePath)
            , m_name(name)
        {
            LoadFile();
        }
        
        void Render(GameContext& context, const ImVec2& availableSize) override
        {
            ImGui::Text("File: %s", m_filePath.c_str());
            ImGui::Separator();
            
            // Display file content in read-only text area
            ImGui::InputTextMultiline("##source", 
                (char*)m_fileContent.c_str(), 
                m_fileContent.size() + 1,
                availableSize,
                ImGuiInputTextFlags_ReadOnly);
        }
        
        std::string GetPreviewName() const override { return m_name; }
        
    private:
        void LoadFile()
        {
            PHYSFS_File* file = PHYSFS_openRead(m_filePath.c_str());
            if (file)
            {
                PHYSFS_sint64 size = PHYSFS_fileLength(file);
                std::string buffer(size, '\0');
                PHYSFS_readBytes(file, buffer.data(), size);
                PHYSFS_close(file);
                m_fileContent = std::move(buffer);
            }
            else
            {
                m_fileContent = "Failed to load file: " + m_filePath;
            }
        }
        
    private:
        std::string m_filePath;
        std::string m_name;
        std::string m_fileContent;
    };
}