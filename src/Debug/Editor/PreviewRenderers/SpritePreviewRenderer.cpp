
#include "SpritePreviewRenderer.h"

#include "Engine/ECS/Component/Sprite.h"
#include "Engine/Resource/TextureResource.h"
#include "rlImGui.h"

namespace Struktur::Debug
{
void SpritePreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	if (!m_sprite)
	{
		ImGui::Text("Invalid sprite");
		return;
	}

	if (!m_sprite->texture->IsGpuReady())
	{
		m_sprite->texture->LoadToGpu();
	}

	Resource::TextureResource* texture = m_sprite->texture.Get();
	const ::Texture2D& tex             = texture->texture;

	ImGui::Text("Texture: %dx%d", tex.width, tex.height);
	ImGui::Text("Sprite Sheet: %dx%d", m_sprite->columns, m_sprite->rows);
	ImGui::Text("Current Index: %d", m_sprite->index);
	ImGui::Text("Color: RGBA(%d,%d,%d,%d)", m_sprite->color.r, m_sprite->color.g, m_sprite->color.b, m_sprite->color.a);
	ImGui::Separator();

	// Calculate source rectangle for current sprite frame
	float frameWidth  = (float)tex.width / m_sprite->columns;
	float frameHeight = (float)tex.height / m_sprite->rows;
	int row           = m_sprite->index / m_sprite->columns;
	int col           = m_sprite->index % m_sprite->columns;

	Rectangle sourceRec = {col * frameWidth + m_sprite->offset.x, row * frameHeight + m_sprite->offset.y,
	                       m_sprite->flipped ? -frameWidth : frameWidth, frameHeight};

	// Calculate display size
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

	// Center the sprite
	ImVec2 cursorPos = ImGui::GetCursorPos();
	cursorPos.x += (availableSize.x - displaySize.x) * 0.5f;
	ImGui::SetCursorPos(cursorPos);

	rlImGuiImageRect(&tex, (int)displaySize.x, (int)displaySize.y, sourceRec);
}
}  // namespace Struktur::Debug
