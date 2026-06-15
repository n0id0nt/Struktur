#include "TexturePreviewRenderer.h"

#include "rlImGui.h"

void Struktur::Debug::TexturePreviewRenderer::Render(GameContext &context, const ImVec2 &availableSize)
{
	if (!m_texture->IsGpuReady())
	{
		m_texture->LoadToGpu();
	}

	const ::Texture2D &tex = m_texture->texture;

	ImGui::Text("Size: %dx%d", tex.width, tex.height);
	ImGui::Text("Format: %d", tex.format);
	ImGui::Separator();

	// Calculate display size
	float aspect = (float)tex.width / (float)tex.height;
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

	rlImGuiImageRect(&tex, (int)displaySize.x, (int)displaySize.y,
	                 Rectangle{0, 0, (float)tex.width, (float)tex.height});
}
