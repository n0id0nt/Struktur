#pragma once

#include "PreviewRenderer.h"
#include <string>

namespace Struktur::Debug
{
    class UnknownFilePreviewRenderer : public IPreviewRenderer
    {
    public:
        UnknownFilePreviewRenderer(const std::string& fileName)
            : m_fileName(fileName)
        {
        }
        
        void Render(GameContext& context, const ImVec2& availableSize) override
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::TextWrapped("Unknown File Type");
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            
            ImGui::Text("File: %s", m_fileName.c_str());
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::TextWrapped("This file type cannot be previewed. The preview system doesn't have a renderer for this file format.");
            
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
                "To add support for this file type, create a new preview renderer class.");
        }
        
        std::string GetPreviewName() const override { return m_fileName; }
        
    private:
        std::string m_fileName;
    };
}
