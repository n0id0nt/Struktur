#include "UIPanel.h"

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UIPanel::UIPanel(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                               const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize),
      m_hasBackgroundTexture(false)
{
	m_backgroundColor = Util::ColorLightGray;
	m_focusable       = false;  // Panels typically don't receive focus
}

void Struktur::UI::UIPanel::SetBackgroundTexture(const Resource::ResourcePtr<Resource::TextureResource>& texture)
{
	m_backgroundTexture    = texture;
	m_hasBackgroundTexture = true;
	m_visualDirty          = true;
}

void Struktur::UI::UIPanel::ClearBackgroundTexture()
{
	m_hasBackgroundTexture = false;
	m_backgroundTexture    = Resource::ResourcePtr<Resource::TextureResource>();
	m_visualDirty          = true;
}

void Struktur::UI::UIPanel::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UIPanel::Render(GameContext& context)
{
	if (m_visualDirty)
	{
		// GetRequiredQuadCount() can change at runtime (border toggled on/off, or via SetBorderWidth) - grow the
		// slot first if it no longer fits, same AllocateOrResizeSlot-before-write contract WriteText already
		// documents for UILabel's variable text length.
		uint32_t requiredQuads = GetRequiredQuadCount();
		if (m_batchSlot.quadCapacity < requiredQuads)
		{
			m_batchSlot = context.GetUIRenderer().AllocateOrResizeSlot(m_batch, m_batchSlot, requiredQuads);
		}

		// Background occupies the slot's first quad; the border outline (if drawn) occupies the next 4 - must
		// match GetRequiredQuadCount()'s own layout (1 + optionally 4) exactly, since every WriteX call below
		// always writes starting at whatever slot it's given, not some running offset. Each write tags its own
		// quads with its own texture (see UIBatch::quadTextures), so the background's real texture and the
		// border's white one can safely coexist in the same batch/slot.
		Renderer::UIBatchSlot backgroundSlot = m_batchSlot;
		backgroundSlot.quadCapacity           = 1;

		if (m_hasBackgroundTexture)
		{
			Resource::TextureResource* texture = m_backgroundTexture.Get();
			if (!texture->IsGpuReady())
			{
				texture->LoadToGpu(context);
			}
			context.GetUIRenderer().WriteTexturedRect(m_batch, backgroundSlot, m_bounds, texture->GetHandle(),
			                                          Util::ColorWhite);
		}
		else
		{
			context.GetUIRenderer().WriteRect(m_batch, backgroundSlot, m_bounds, m_backgroundColor);
		}

		uint32_t quadsUsed = 1;
		if (m_borderWidth > 0)
		{
			Renderer::UIBatchSlot borderSlot = m_batchSlot;
			borderSlot.vertexOffset += 4;  // past the background's 1 quad (4 vertices)
			borderSlot.quadCapacity = 4;
			context.GetUIRenderer().WriteRectOutline(m_batch, borderSlot, m_bounds, m_borderWidth, m_borderColor);
			quadsUsed += 4;
		}

		// Border can toggle off (SetBorderWidth(0)) after previously being on - without this, its 4 quads would
		// keep rendering their last real outline forever, since Flush() draws the slot's whole allocated
		// capacity regardless of whether this frame's writes above actually touched all of it (same reasoning as
		// UILabel::Render()'s own trailing ClearSlotFrom call for its variable-length text).
		context.GetUIRenderer().ClearSlotFrom(m_batch, m_batchSlot, quadsUsed);

		m_visualDirty = false;
	}

	RenderChildren(context);
}

uint32_t Struktur::UI::UIPanel::GetRequiredQuadCount() const
{
	return 1 + (m_borderWidth > 0 ? 4 : 0);
}
