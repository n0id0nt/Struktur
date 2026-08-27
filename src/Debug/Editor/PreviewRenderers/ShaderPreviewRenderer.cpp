
#include "ShaderPreviewRenderer.h"

#include "Engine/ECS/Component/Shader.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
// This preview renders through raylib directly (BeginTextureMode/DrawTexturePro/BeginShaderMode), which has no
// initialised window/rlgl context to run against now that bgfx renders the game - stubbed until the
// ImGui-bgfx backend gets a bgfx-based version of this preview.
void ShaderPreviewRenderer::InitialisePreview() {}
void ShaderPreviewRenderer::CleanupPreview() {}

void ShaderPreviewRenderer::RenderControls(GameContext& context)
{
	// Shape selection
	const char* shapes[] = {"2D Quad", "Cube", "Sphere"};
	int currentShape     = (int)m_previewShape;

	if (ImGui::Combo("Preview Shape", &currentShape, shapes, 3))
	{
		m_previewShape = (ShaderPreviewShape)currentShape;
	}

	// Rotation control
	ImGui::SliderFloat("Rotation", &m_rotation, 0.0f, 360.0f);

	ImGui::Checkbox("Use Custom Texture", &m_useCustomTexture);

	if (ImGui::Button("Reset View"))
	{
		m_rotation = 0.0f;
	}

	ImGui::Separator();
}

void ShaderPreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	ImGui::Text("Shader preview unavailable (bgfx editor backend pending)");
}

void ShaderPreviewRenderer::RenderShaderPreview(const ImVec2& size) {}
void ShaderPreviewRenderer::ApplyShaderUniforms() {}
}  // namespace Struktur::Debug
