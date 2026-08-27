#include "TextFilePreviewRenderer.h"

#include <imgui.h>

#include "Engine/Core/FileSystem.h"

namespace Struktur::Debug
{
void TextFilePreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	ImGui::Text("File: %s", m_filePath.c_str());
	ImGui::Separator();

	// Display file content in read-only text area
	ImGui::InputTextMultiline("##source", (char*)m_fileContent.c_str(), m_fileContent.size() + 1, availableSize,
	                          ImGuiInputTextFlags_ReadOnly);
}

void TextFilePreviewRenderer::LoadFile()
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
}  // namespace Struktur::Debug
