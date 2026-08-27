#include "SpritePreviewRenderer.h"

#include <algorithm>

#include "Engine/ECS/Component/Sprite.h"
#include "Engine/Resource/TextureResource.h"

void Struktur::Debug::SpritePreviewRenderer::RenderControls(GameContext& context)
{
	if (!m_sprite)
	{
		return;
	}

	if (ImGui::RadioButton("Current Frame", m_mode == Mode::CurrentSprite))
	{
		m_mode = Mode::CurrentSprite;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Specified Frame", m_mode == Mode::SpecifiedSprite))
	{
		m_mode = Mode::SpecifiedSprite;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Whole Sheet", m_mode == Mode::EntireSheet))
	{
		m_mode = Mode::EntireSheet;
	}

	if (m_mode == Mode::SpecifiedSprite)
	{
		int frameCount = std::max(1, m_sprite->columns * m_sprite->rows);
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragInt("Frame Index", &m_specifiedIndex, 1.0f, 0, frameCount - 1);
	}

	ImGui::Separator();
}

void Struktur::Debug::SpritePreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	if (!m_sprite)
	{
		ImGui::Text("Invalid sprite");
		return;
	}

	Resource::TextureResource* texture = m_sprite->texture.Get();
	if (!texture)
	{
		ImGui::Text("No texture assigned");
		return;
	}
	if (!texture->IsGpuReady())
	{
		texture->LoadToGpu(context);
	}

	ImGui::Text("Texture: %dx%d", texture->GetWidth(), texture->GetHeight());
	ImGui::Text("Sprite Sheet: %d columns x %d rows", m_sprite->columns, m_sprite->rows);
	ImGui::Text("Current Index: %d", m_sprite->index);
	ImGui::Text("Color: RGBA(%d,%d,%d,%d)", m_sprite->color.r, m_sprite->color.g, m_sprite->color.b,
	           m_sprite->color.a);
	ImGui::Separator();

	switch (m_mode)
	{
	case Mode::CurrentSprite:
		RenderSingleFrame(context, availableSize, m_sprite->index);
		break;
	case Mode::SpecifiedSprite:
		RenderSingleFrame(context, availableSize, m_specifiedIndex);
		break;
	case Mode::EntireSheet:
		RenderWholeSheet(context, availableSize);
		break;
	}
}

void Struktur::Debug::SpritePreviewRenderer::RenderSingleFrame(GameContext& context, const ImVec2& availableSize,
                                                                int index)
{
	Resource::TextureResource* texture = m_sprite->texture.Get();
	int texWidth                       = texture->GetWidth();
	int texHeight                      = texture->GetHeight();

	// Same column/row math System::SpriteRenderSystem actually draws with (see its Render()) - deliberately does
	// NOT add sprite->offset here, unlike this preview's previous implementation: offset is a world-space pivot
	// passed to WorldRenderer::SubmitSprite as `origin`, used to shift destRec, not a shift into the sheet's
	// source rect. Adding it here was the "source rect not positioned correctly" bug - it made the previewed
	// frame drift away from the actual on-screen frame for any sprite with a nonzero offset.
	float frameWidth  = (float)texWidth / m_sprite->columns;
	float frameHeight = (float)texHeight / m_sprite->rows;
	int clampedIndex  = std::clamp(index, 0, std::max(0, m_sprite->columns * m_sprite->rows - 1));
	int row           = clampedIndex / m_sprite->columns;
	int col           = clampedIndex % m_sprite->columns;

	float srcX = col * frameWidth;
	float srcY = row * frameHeight;
	float srcW = m_sprite->flipped ? -frameWidth : frameWidth;
	float srcH = frameHeight;

	float aspect = frameWidth / frameHeight;
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

	ImVec2 cursorPos = ImGui::GetCursorPos();
	cursorPos.x += (availableSize.x - displaySize.x) * 0.5f;
	ImGui::SetCursorPos(cursorPos);

	ImVec2 uv0(srcX / texWidth, srcY / texHeight);
	ImVec2 uv1((srcX + srcW) / texWidth, (srcY + srcH) / texHeight);

	Renderer::TextureHandle handle = texture->GetHandle();
	ImGui::Image((ImTextureID)(intptr_t)handle.id, displaySize, uv0, uv1);
}

void Struktur::Debug::SpritePreviewRenderer::RenderWholeSheet(GameContext& context, const ImVec2& availableSize)
{
	Resource::TextureResource* texture = m_sprite->texture.Get();
	int texWidth                       = texture->GetWidth();
	int texHeight                      = texture->GetHeight();

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

	ImVec2 cursorPos = ImGui::GetCursorPos();
	cursorPos.x += (availableSize.x - displaySize.x) * 0.5f;
	ImGui::SetCursorPos(cursorPos);

	ImVec2 imageScreenPos = ImGui::GetCursorScreenPos();
	Renderer::TextureHandle handle = texture->GetHandle();
	ImGui::Image((ImTextureID)(intptr_t)handle.id, displaySize);

	// Grid + current-frame highlight, drawn in screen space over the just-placed image - lets a misconfigured
	// columns/rows (or the offset bug fixed in RenderSingleFrame) be spotted visually against the real sheet
	// instead of only from the raw numbers above.
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float cellWidth      = displaySize.x / (float)m_sprite->columns;
	float cellHeight     = displaySize.y / (float)m_sprite->rows;

	ImU32 gridColor = IM_COL32(255, 255, 255, 80);
	for (int col = 1; col < m_sprite->columns; ++col)
	{
		float x = imageScreenPos.x + col * cellWidth;
		drawList->AddLine(ImVec2(x, imageScreenPos.y), ImVec2(x, imageScreenPos.y + displaySize.y), gridColor);
	}
	for (int row = 1; row < m_sprite->rows; ++row)
	{
		float y = imageScreenPos.y + row * cellHeight;
		drawList->AddLine(ImVec2(imageScreenPos.x, y), ImVec2(imageScreenPos.x + displaySize.x, y), gridColor);
	}

	int frameCount = m_sprite->columns * m_sprite->rows;
	if (frameCount > 0)
	{
		int clampedIndex = std::clamp(m_sprite->index, 0, frameCount - 1);
		int row          = clampedIndex / m_sprite->columns;
		int col          = clampedIndex % m_sprite->columns;

		ImVec2 highlightMin(imageScreenPos.x + col * cellWidth, imageScreenPos.y + row * cellHeight);
		ImVec2 highlightMax(highlightMin.x + cellWidth, highlightMin.y + cellHeight);
		drawList->AddRect(highlightMin, highlightMax, IM_COL32(255, 220, 0, 255), 0.0f, 0, 2.0f);
	}

	ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.0f, 1.0f), "Highlighted cell = Current Index (%d)", m_sprite->index);
}
