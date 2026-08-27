#include "PreviewWindow.h"

#include "Engine/GameContext.h"

namespace Struktur::Debug
{
void PreviewWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	if (!m_currentRenderer)
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No preview available");
		ImGui::Text("Select an asset or component to preview");
		ImGui::End();
		return;
	}

	// Show preview name
	ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", m_currentRenderer->GetPreviewName().c_str());
	ImGui::Separator();

	// Render controls (if any)
	m_currentRenderer->RenderControls(context);

	// Get available size for content
	ImVec2 availableSize = ImGui::GetContentRegionAvail();

	// Render preview content
	m_currentRenderer->Render(context, availableSize);

	ImGui::End();
}
}  // namespace Struktur::Debug
