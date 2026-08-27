#pragma once

#include "Engine/UI/UIElement.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIClip - Masks its children to its own bounds via UIRenderer's per-batch scissor clip. Draws nothing of its
// own (GetRequiredQuadCount() == 0) - purely a clipping boundary. Always a batch root (SetBatchRoot(true) in the
// constructor), since clipping is strictly per-batch (see UIRenderer::SetBatchClip) - a subtree needs its own
// batch to be clippable independent of whatever it's nested inside.
//
// UIScroll builds on top of this rather than duplicating the clipping concern - see UIScroll.h.
//=============================================================================
class UIClip : public UIElement
{
public:
	UIClip(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	       const glm::vec2& relativeSize);

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// Draws no quads of its own - purely a clipping boundary around its children.
	uint32_t GetRequiredQuadCount() const override
	{
		return 0;
	}

	bool ClipsChildren() const override
	{
		return true;
	}
};
}  // namespace UI
}  // namespace Struktur
