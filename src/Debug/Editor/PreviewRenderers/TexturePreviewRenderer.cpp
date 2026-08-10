#include "TexturePreviewRenderer.h"

void Struktur::Debug::TexturePreviewRenderer::Render(GameContext &context, const ImVec2 &availableSize)
{
	if (!m_texture->IsGpuReady())
	{
		m_texture->LoadToGpu(context);
	}

	int texWidth  = m_texture->GetWidth();
	int texHeight = m_texture->GetHeight();
	ImGui::Text("Size: %dx%d", texWidth, texHeight);
	ImGui::Separator();

	// Calculate display size
	float aspect = (float)texWidth / (float)texHeight;
	ImVec2 displaySize;

	if (availableSize.x / aspect < availableSize.y)
	{
		displaySize.x = availableSize.x;
		displaySize.y = availableSize.x / aspect;
	}
	else
	{
		displaySize.y = availableSize.y;
		displaySize.x = availableSize.y * aspect;
	}

	// Center the image
	ImVec2 cursorPos = ImGui::GetCursorPos();
	cursorPos.x += (availableSize.x - displaySize.x) * 0.5f;
	ImGui::SetCursorPos(cursorPos);

	Struktur::Renderer::TextureHandle handle = m_texture->GetHandle();
	ImGui::Image((ImTextureID)(intptr_t)handle.id, displaySize);
}
