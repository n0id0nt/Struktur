#include "ImGuiRenderer.h"

#if !defined(PLATFORM_WEB)

	#include <cstring>

	#include "Debug/Assertions.h"
	#include "Engine/Renderer/EmbeddedShaders.h"
	#include "Engine/Renderer/GraphicsDevice.h"
	#include "Engine/Renderer/SpriteVertex.h"

	#include <glm/gtc/matrix_transform.hpp>
	#include <glm/gtc/type_ptr.hpp>
	#include <imgui.h>

Struktur::Renderer::ImGuiRenderer::ImGuiRenderer()
{
	m_texColorSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

	// Old single-atlas texture model (this vendored ImGui predates the dynamic ImTextureData API) - build one
	// bgfx texture up front and hand its index back to ImGui as the font atlas's ImTextureID, the same way
	// rlImGui casts a raw raylib texture id into one; every other texture (viewport, previews) does the same.
	ImGuiIO& io           = ImGui::GetIO();
	unsigned char* pixels = nullptr;
	int width = 0, height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	m_fontTexture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA8, 0,
	                                      bgfx::copy(pixels, (uint32_t)(width * height * 4)));
	ASSERT_MSG(bgfx::isValid(m_fontTexture), "Failed to create ImGui font atlas texture");
	io.Fonts->TexID = (ImTextureID)(intptr_t)m_fontTexture.idx;
}

Struktur::Renderer::ImGuiRenderer::~ImGuiRenderer()
{
	if (bgfx::isValid(m_fontTexture))
	{
		bgfx::destroy(m_fontTexture);
	}
}

void Struktur::Renderer::ImGuiRenderer::Render(ImDrawData* drawData)
{
	if (!drawData || drawData->CmdListsCount == 0)
	{
		return;
	}

	int fbWidth  = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fbWidth <= 0 || fbHeight <= 0)
	{
		return;
	}

	// Orthographic screen-space projection sized to the display, no camera involved (unlike the world view).
	glm::mat4 identity(1.0f);
	glm::mat4 proj = glm::ortho(drawData->DisplayPos.x, drawData->DisplayPos.x + drawData->DisplaySize.x,
	                            drawData->DisplayPos.y + drawData->DisplaySize.y, drawData->DisplayPos.y, -1.0f, 1.0f);
	bgfx::setViewTransform(GraphicsDevice::EditorViewId, glm::value_ptr(identity), glm::value_ptr(proj));

	static const bgfx::VertexLayout layout = BuildSpriteVertexLayout();
	bgfx::ProgramHandle program              = GetEmbeddedProgram("sprite");
	ImVec2 clipOff                           = drawData->DisplayPos;
	ImVec2 clipScale                         = drawData->FramebufferScale;

	for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex)
	{
		const ImDrawList* drawList = drawData->CmdLists[listIndex];

		uint32_t numVertices = (uint32_t)drawList->VtxBuffer.Size;
		uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.Size;
		if (numVertices == 0 || numIndices == 0)
		{
			continue;
		}
		if (numVertices > bgfx::getAvailTransientVertexBuffer(numVertices, layout) ||
		    numIndices > bgfx::getAvailTransientIndexBuffer(numIndices))
		{
			continue;
		}

		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;
		bgfx::allocTransientVertexBuffer(&tvb, numVertices, layout);
		bgfx::allocTransientIndexBuffer(&tib, numIndices);

		// ImDrawVert (pos:2f, uv:2f, col:u32) matches SpriteVertex byte-for-byte - a straight bulk copy, no
		// per-vertex conversion needed (see the class-level comment).
		std::memcpy(tvb.data, drawList->VtxBuffer.Data, numVertices * sizeof(ImDrawVert));
		std::memcpy(tib.data, drawList->IdxBuffer.Data, numIndices * sizeof(ImDrawIdx));

		for (int cmdIndex = 0; cmdIndex < drawList->CmdBuffer.Size; ++cmdIndex)
		{
			const ImDrawCmd& cmd = drawList->CmdBuffer[cmdIndex];
			if (cmd.UserCallback)
			{
				cmd.UserCallback(drawList, &cmd);
				continue;
			}
			if (cmd.ElemCount == 0)
			{
				continue;
			}

			ImVec2 clipMin((cmd.ClipRect.x - clipOff.x) * clipScale.x, (cmd.ClipRect.y - clipOff.y) * clipScale.y);
			ImVec2 clipMax((cmd.ClipRect.z - clipOff.x) * clipScale.x, (cmd.ClipRect.w - clipOff.y) * clipScale.y);
			if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
			{
				continue;
			}

			bgfx::setScissor((uint16_t)clipMin.x, (uint16_t)clipMin.y, (uint16_t)(clipMax.x - clipMin.x),
			                 (uint16_t)(clipMax.y - clipMin.y));

			bgfx::TextureHandle texture{(uint16_t)(intptr_t)cmd.GetTexID()};
			bgfx::setTexture(0, m_texColorSampler, texture);
			bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
			bgfx::setIndexBuffer(&tib, cmd.IdxOffset, cmd.ElemCount);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
			               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
			bgfx::submit(GraphicsDevice::EditorViewId, program);
		}
	}
}

#endif
