
#include "SpritePreviewRenderer.h"

#include "Engine/ECS/Component/Sprite.h"
#include "Engine/Resource/TextureResource.h"

#if defined(PLATFORM_WEB)
	#include "rlImGui.h"
#endif

namespace Struktur::Debug
{
#if !defined(PLATFORM_WEB)
void SpritePreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	if (!m_sprite)
	{
		ImGui::Text("Invalid sprite");
		return;
	}

	Resource::TextureResource* texture = m_sprite->texture.Get();
	if (!texture->IsGpuReady())
	{
		texture->LoadToGpu(context);
	}

	int texWidth  = texture->GetWidth();
	int texHeight = texture->GetHeight();

	ImGui::Text("Texture: %dx%d", texWidth, texHeight);
	ImGui::Text("Sprite Sheet: %dx%d", m_sprite->columns, m_sprite->rows);
	ImGui::Text("Current Index: %d", m_sprite->index);
	ImGui::Text("Color: RGBA(%d,%d,%d,%d)", m_sprite->color.r, m_sprite->color.g, m_sprite->color.b, m_sprite->color.a);
	ImGui::Separator();

	// Calculate source rectangle for current sprite frame
	float frameWidth  = (float)texWidth / m_sprite->columns;
	float frameHeight = (float)texHeight / m_sprite->rows;
	int row           = m_sprite->index / m_sprite->columns;
	int col           = m_sprite->index % m_sprite->columns;

	float srcX = col * frameWidth + m_sprite->offset.x;
	float srcY = row * frameHeight + m_sprite->offset.y;
	float srcW = m_sprite->flipped ? -frameWidth : frameWidth;
	float srcH = frameHeight;

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

	ImVec2 uv0(srcX / texWidth, srcY / texHeight);
	ImVec2 uv1((srcX + srcW) / texWidth, (srcY + srcH) / texHeight);

	Renderer::TextureHandle handle = texture->GetHandle();
	ImGui::Image((ImTextureID)(intptr_t)handle.id, displaySize, uv0, uv1);
}
#else
void SpritePreviewRenderer::Render(GameContext& context, const ImVec2& availableSize)
{
	if (!m_sprite)
	{
		ImGui::Text("Invalid sprite");
		return;
	}

	if (!m_sprite->texture->IsGpuReady())
	{
		m_sprite->texture->LoadToGpu(context);
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
#endif
}  // namespace Struktur::Debug
