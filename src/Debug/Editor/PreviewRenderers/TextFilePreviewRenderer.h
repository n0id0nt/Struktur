#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include "Engine/Core/FileSystem.h"
#include "PreviewRenderer.h"
#include "physfs.h"

namespace Struktur::Debug
{
class TextFilePreviewRenderer : public IPreviewRenderer
{
   public:
	TextFilePreviewRenderer(const std::string& filePath, const std::string& name)
	    : m_filePath(filePath),
	      m_name(name)
	{
		LoadFile();
	}

	void Render(GameContext& context, const ImVec2& availableSize) override
	{
		ImGui::Text("File: %s", m_filePath.c_str());
		ImGui::Separator();

		// Display file content in read-only text area
		ImGui::InputTextMultiline("##source", (char*)m_fileContent.c_str(), m_fileContent.size() + 1, availableSize,
		                          ImGuiInputTextFlags_ReadOnly);
	}

	std::string GetPreviewName() const override
	{
		return m_name;
	}

   private:
	void LoadFile()
	{
		auto result = FileSystem::ReadString(m_filePath);
		if (result)
		{
			m_fileContent = std::move(result.value);
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
}  // namespace Struktur::Debug
