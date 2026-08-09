
#include "ShaderPreviewRenderer.h"

#include "Engine/ECS/Component/Shader.h"
#include "Engine/GameContext.h"
#if defined(PLATFORM_WEB)
	#include "rlImGui.h"
#endif

namespace Struktur::Debug
{
#if !defined(PLATFORM_WEB)
// This preview renders through raylib directly (BeginTextureMode/DrawTexturePro/BeginShaderMode), which has no
// initialised window/rlgl context to run against now that bgfx renders the game - stubbed until the
// ImGui-bgfx backend (and a bgfx-based version of this preview) lands.
void ShaderPreviewRenderer::InitialisePreview() {}
void ShaderPreviewRenderer::CleanupPreview() {}
#else
void ShaderPreviewRenderer::InitialisePreview()
{
	// Create a default checkerboard texture
	const int texSize  = 256;
	Image checkerboard = GenImageChecked(texSize, texSize, 32, 32, LIGHTGRAY, DARKGRAY);
	m_previewTexture   = LoadTextureFromImage(checkerboard);
	UnloadImage(checkerboard);

	// Create render texture for preview
	m_renderTexture = LoadRenderTexture(512, 512);
}

void ShaderPreviewRenderer::CleanupPreview()
{
	if (m_previewTexture.id != 0)
	{
		UnloadTexture(m_previewTexture);
		m_previewTexture.id = 0;
	}

	if (m_renderTexture.id != 0)
	{
		UnloadRenderTexture(m_renderTexture);
		m_renderTexture.id = 0;
	}
}
#endif

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

#if !defined(PLATFORM_WEB)
void ShaderPreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	ImGui::Text("Shader preview unavailable (bgfx editor backend pending)");
}
#else
void ShaderPreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	if (!m_shader)
	{
		ImGui::Text("Invalid shader");
		return;
	}

	// Render shader preview to texture
	RenderShaderPreview(availableSize);

	// Display the rendered texture
	ImVec2 displaySize = availableSize;

	// Center the preview
	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(cursorPos);

	rlImGuiImageRect(&m_renderTexture.texture, (int)displaySize.x, (int)displaySize.y,
	                 Rectangle{0, 0, (float)m_renderTexture.texture.width, -(float)m_renderTexture.texture.height});
}
#endif

#if defined(PLATFORM_WEB)
void ShaderPreviewRenderer::RenderShaderPreview(const ImVec2& size)
{
	BeginTextureMode(m_renderTexture);
	ClearBackground(Color{40, 40, 50, 255});

	// Begin shader mode
	BeginShaderMode(m_shader->shader->shader);

	// Apply all uniforms
	ApplyShaderUniforms();

	// Render based on shape
	switch (m_previewShape)
	{
		case ShaderPreviewShape::Quad2D:
		{
			// Draw a centered quad with the texture
			Rectangle dest   = {m_renderTexture.texture.width / 2.0f - 200, m_renderTexture.texture.height / 2.0f - 200,
			                    400, 400};
			Rectangle source = {0, 0, (float)m_previewTexture.width, (float)m_previewTexture.height};
			DrawTexturePro(m_previewTexture, source, dest, Vector2{200, 200}, m_rotation, WHITE);
			break;
		}

		case ShaderPreviewShape::Cube:
		{
			// Draw a 3D cube (would need 3D camera setup)
			// For now, just draw a textured quad representing a cube face
			Rectangle dest = {m_renderTexture.texture.width / 2.0f - 150, m_renderTexture.texture.height / 2.0f - 150,
			                  300, 300};
			DrawRectangleRec(dest, WHITE);
			ImGui::Text("Cube preview - 3D rendering not implemented");
			break;
		}

		case ShaderPreviewShape::Sphere:
		{
			// Draw a sphere (would need 3D rendering)
			float centerX = m_renderTexture.texture.width / 2.0f;
			float centerY = m_renderTexture.texture.height / 2.0f;
			DrawCircle((int)centerX, (int)centerY, 150, WHITE);
			break;
		}
	}

	EndShaderMode();
	EndTextureMode();
}

void ShaderPreviewRenderer::ApplyShaderUniforms()
{
	if (!m_shader)
	{
		return;
	}

	if (!m_shader->shader->IsGpuResourceValid())
	{
		return;
	}

	// Apply float uniforms
	for (const auto& [name, value] : m_shader->floatUniforms)
	{
		int location = GetShaderLocation(m_shader->shader->shader, name.c_str());
		if (location != -1)
		{
			SetShaderValue(m_shader->shader->shader, location, &value, SHADER_UNIFORM_FLOAT);
		}
	}

	// Apply int uniforms
	for (const auto& [name, value] : m_shader->intUniforms)
	{
		int location = GetShaderLocation(m_shader->shader->shader, name.c_str());
		if (location != -1)
		{
			SetShaderValue(m_shader->shader->shader, location, &value, SHADER_UNIFORM_INT);
		}
	}

	// Apply vec2 uniforms
	for (const auto& [name, value] : m_shader->vec2Uniforms)
	{
		int location = GetShaderLocation(m_shader->shader->shader, name.c_str());
		if (location != -1)
		{
			SetShaderValue(m_shader->shader->shader, location, &value, SHADER_UNIFORM_VEC2);
		}
	}

	// Apply vec3 uniforms
	for (const auto& [name, value] : m_shader->vec3Uniforms)
	{
		int location = GetShaderLocation(m_shader->shader->shader, name.c_str());
		if (location != -1)
		{
			SetShaderValue(m_shader->shader->shader, location, &value, SHADER_UNIFORM_VEC3);
		}
	}

	// Apply vec4 uniforms
	for (const auto& [name, value] : m_shader->vec4Uniforms)
	{
		int location = GetShaderLocation(m_shader->shader->shader, name.c_str());
		if (location != -1)
		{
			SetShaderValue(m_shader->shader->shader, location, &value, SHADER_UNIFORM_VEC4);
		}
	}

	// Apply matrix uniforms
	for (const auto& [name, value] : m_shader->matrixUniforms)
	{
		int location = GetShaderLocation(m_shader->shader->shader, name.c_str());
		if (location != -1)
		{
			SetShaderValueMatrix(m_shader->shader->shader, location, value);
		}
	}
}
#else
void ShaderPreviewRenderer::RenderShaderPreview(const ImVec2& size) {}
void ShaderPreviewRenderer::ApplyShaderUniforms() {}
#endif
}  // namespace Struktur::Debug
