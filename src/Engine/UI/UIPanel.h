#pragma once

#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIPanel - Container element
//=============================================================================
class UIPanel : public UIElement
{
   private:
	Resource::ResourcePtr<Resource::TextureResource> m_backgroundTexture;
	bool m_hasBackgroundTexture;

   public:
	UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	        const glm::vec2& relativeSize);

	void SetBackgroundTexture(const Resource::ResourcePtr<Resource::TextureResource>& texture);
	void ClearBackgroundTexture();

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// 1 quad for the background (DrawRect/DrawTexturedRect) plus 4 for the border outline (DrawRectOutline) when
	// one is actually drawn - mirrors Render()'s own draw calls exactly.
	uint32_t GetRequiredQuadCount() const override;
};
}  // namespace UI
}  // namespace Struktur
