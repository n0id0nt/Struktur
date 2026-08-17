#include "UIRenderer.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Debug/Assertions.h"
#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/EmbeddedShaders.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/QuadVertex.h"
#include "Engine/UI/UIElement.h"

void Struktur::Renderer::UIRenderer::Initialise()
{
	m_texColorSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

	uint8_t whitePixel[4] = {255, 255, 255, 255};
	m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(whitePixel, 4));
	ASSERT_MSG(bgfx::isValid(m_whiteTexture), "Failed to create UI white texture");
}

void Struktur::Renderer::UIRenderer::Shutdown()
{
	if (bgfx::isValid(m_whiteTexture))
	{
		bgfx::destroy(m_whiteTexture);
		m_whiteTexture = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(m_texColorSampler))
	{
		bgfx::destroy(m_texColorSampler);
		m_texColorSampler = BGFX_INVALID_HANDLE;
	}
}

void Struktur::Renderer::UIRenderer::SetupView(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	glm::mat4 identity(1.0f);
	// Screen-space, top-left origin, sized to the game's logical resolution (not the real window) - matches
	// WorldRenderer's camera projection sizing convention so UI lines up with the same target when
	// GraphicsDevice::SetWorldRenderTarget redirects both views into the editor's Game Viewport framebuffer.
	glm::mat4 proj = glm::ortho(0.0f, (float)gameData.gameWidth, (float)gameData.gameHeight, 0.0f, -1.0f, 1.0f);
	bgfx::setViewTransform(GraphicsDevice::UIViewId, glm::value_ptr(identity), glm::value_ptr(proj));
}

namespace
{
// Fills one quad's 4 vertices (position/UV/packed color), CCW starting top-left - shared by the transient
// immediate-submit path (SubmitTexturedQuad) and the persistent batch-write path (WriteRect/WriteRectOutline/
// WriteTexturedRect/WriteText below), so this layout/winding only exists in one place.
void FillQuadVertices(Struktur::Renderer::QuadVertex* quad, float x, float y, float w, float h, float u0, float v0,
                      float u1, float v1, uint32_t abgr)
{
	quad[0] = {x, y, u0, v0, abgr};
	quad[1] = {x + w, y, u1, v0, abgr};
	quad[2] = {x + w, y + h, u1, v1, abgr};
	quad[3] = {x, y + h, u0, v1, abgr};
}
}  // namespace

void Struktur::Renderer::UIRenderer::SubmitTexturedQuad(float x, float y, float w, float h, float u0, float v0,
                                                        float u1, float v1, uint32_t abgr, bgfx::TextureHandle texture)
{
	static const bgfx::VertexLayout layout = BuildQuadVertexLayout();
	if (4 > bgfx::getAvailTransientVertexBuffer(4, layout) || 6 > bgfx::getAvailTransientIndexBuffer(6))
	{
		return;
	}

	bgfx::TransientVertexBuffer tvb;
	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
	bgfx::allocTransientIndexBuffer(&tib, 6);

	FillQuadVertices((QuadVertex*)tvb.data, x, y, w, h, u0, v0, u1, v1, abgr);

	uint16_t* indices = (uint16_t*)tib.data;
	indices[0]        = 0;
	indices[1]        = 1;
	indices[2]        = 2;
	indices[3]        = 0;
	indices[4]        = 2;
	indices[5]        = 3;

	bgfx::setTexture(0, m_texColorSampler, texture);
	bgfx::setVertexBuffer(0, &tvb, 0, 4);
	bgfx::setIndexBuffer(&tib, 0, 6);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
	bgfx::submit(GraphicsDevice::UIViewId, GetEmbeddedProgram("sprite"));
}

void Struktur::Renderer::UIRenderer::DrawRect(const Util::Math::Rect& rect, const Util::Color& color)
{
	SubmitTexturedQuad(rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, 1.0f, 1.0f, PackColor(color),
	                   m_whiteTexture);
}

void Struktur::Renderer::UIRenderer::DrawRectOutline(const Util::Math::Rect& rect, float thickness,
                                                     const Util::Color& color)
{
	DrawRect(Util::Math::Rect{rect.x, rect.y, rect.width, thickness}, color);                            // top
	DrawRect(Util::Math::Rect{rect.x, rect.y + rect.height - thickness, rect.width, thickness}, color);  // bottom
	DrawRect(Util::Math::Rect{rect.x, rect.y, thickness, rect.height}, color);                           // left
	DrawRect(Util::Math::Rect{rect.x + rect.width - thickness, rect.y, thickness, rect.height}, color);  // right
}

void Struktur::Renderer::UIRenderer::DrawTexturedRect(const Util::Math::Rect& rect, const TextureHandle& texture,
                                                      const Util::Color& tint)
{
	SubmitTexturedQuad(rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, 1.0f, 1.0f, PackColor(tint),
	                   bgfx::TextureHandle{(uint16_t)texture.id});
}

void Struktur::Renderer::UIRenderer::DrawText(const Text::Font& font, const std::string& text,
                                              const glm::vec2& position, float fontSize, const Util::Color& color)
{
	// Stub/unloaded font (see FontResource) - draw nothing rather than divide by zero below.
	if (font.baseSize <= 0 || font.glyphs.empty())
	{
		return;
	}

	float scale               = fontSize / (float)font.baseSize;
	float atlasWidth          = (float)font.texture.width;
	float atlasHeight         = (float)font.texture.height;
	uint32_t abgr             = PackColor(color);
	bgfx::TextureHandle atlas = {(uint16_t)font.texture.id};

	float penX = position.x;
	int index  = 0;
	int size   = (int)text.size();
	while (index < size)
	{
		int codepointByteCount = 0;
		int codepoint          = Text::GetCodepointNext(&text[index], &codepointByteCount);
		int glyphIndex         = Text::GetGlyphIndex(font, codepoint);
		index += codepointByteCount;

		const Text::Glyph& glyph    = font.glyphs[glyphIndex];
		const Util::Math::Rect& rec = glyph.rec;

		if (codepoint != ' ' && codepoint != '\t')
		{
			float dstX = penX + (float)glyph.offsetX * scale;
			float dstY = position.y + (float)glyph.offsetY * scale;
			SubmitTexturedQuad(dstX, dstY, rec.width * scale, rec.height * scale, rec.x / atlasWidth,
			                   rec.y / atlasHeight, (rec.x + rec.width) / atlasWidth,
			                   (rec.y + rec.height) / atlasHeight, abgr, atlas);
		}

		penX += (float)(glyph.advanceX > 0 ? glyph.advanceX : (int)rec.width) * scale;
	}
}

// --- Batch storage (phase 1 - see UIRenderer's class comment; nothing calls these yet) ---

Struktur::Renderer::UIBatchHandle Struktur::Renderer::UIRenderer::CreateBatch()
{
	uint32_t index;
	if (!m_freeBatchSlots.empty())
	{
		index = m_freeBatchSlots.back();
		m_freeBatchSlots.pop_back();
	}
	else
	{
		index = (uint32_t)m_batches.size();
		m_batches.emplace_back();
	}

	// Reset to a fresh, fully-zeroed UIBatch regardless of which path above was taken - UIBatch::clipRect has
	// no in-class default initializer, so a plain default-constructed UIBatch would leave it indeterminate.
	UIBatch& batch = m_batches[index];
	batch          = UIBatch{};
	batch.active   = true;

	static const bgfx::VertexLayout spriteLayout = BuildQuadVertexLayout();
	batch.vb = bgfx::createDynamicVertexBuffer(kInitialBatchQuadCapacity * 4, spriteLayout, BGFX_BUFFER_ALLOW_RESIZE);
	batch.ib = bgfx::createDynamicIndexBuffer(kInitialBatchQuadCapacity * 6, BGFX_BUFFER_ALLOW_RESIZE);
	ASSERT_MSG(bgfx::isValid(batch.vb), "Failed to create UI batch vertex buffer");
	ASSERT_MSG(bgfx::isValid(batch.ib), "Failed to create UI batch index buffer");

	return UIBatchHandle{index};
}

void Struktur::Renderer::UIRenderer::DestroyBatch(UIBatchHandle handle)
{
	ASSERT_MSG(handle.IsValid() && handle.index < m_batches.size(), "DestroyBatch: invalid batch handle (index %u)",
	           handle.index);
	if (!handle.IsValid() || handle.index >= m_batches.size())
	{
		return;
	}

	UIBatch& batch = m_batches[handle.index];
	ASSERT_MSG(batch.active, "DestroyBatch: batch %u already destroyed (double-destroy)", handle.index);
	if (!batch.active)
	{
		return;
	}

	if (bgfx::isValid(batch.vb))
	{
		bgfx::destroy(batch.vb);
	}
	if (bgfx::isValid(batch.ib))
	{
		bgfx::destroy(batch.ib);
	}

	batch = UIBatch{};
	m_freeBatchSlots.push_back(handle.index);
}

void Struktur::Renderer::UIRenderer::SetBatchDrawOrder(UIBatchHandle handle, int32_t drawOrder)
{
	ASSERT_MSG(handle.IsValid() && handle.index < m_batches.size(),
	           "SetBatchDrawOrder: invalid batch handle (index %u)", handle.index);
	if (!handle.IsValid() || handle.index >= m_batches.size())
	{
		return;
	}
	m_batches[handle.index].drawOrder = drawOrder;
}

Struktur::Renderer::UIBatchSlot Struktur::Renderer::UIRenderer::AllocateOrResizeSlot(UIBatchHandle batchHandle,
                                                                                     UIBatchSlot existing,
                                                                                     uint32_t quadCount)
{
	ASSERT_MSG(batchHandle.IsValid() && batchHandle.index < m_batches.size(),
	           "AllocateOrResizeSlot: invalid batch handle (index %u)", batchHandle.index);
	if (!batchHandle.IsValid() || batchHandle.index >= m_batches.size())
	{
		return existing;
	}
	UIBatch& batch = m_batches[batchHandle.index];

	if (existing.quadCapacity >= quadCount)
	{
		existing.quadCount = quadCount;
		batch.dirty        = true;
		return existing;
	}

	// Relocate to a fresh region appended at the end of cpuVertices rather than growing in place - the old
	// region at existing.vertexOffset is abandoned (not reclaimed/compacted), so its space is wasted until this
	// batch is next destroyed. Acceptable per the original design note; what's NOT acceptable is leaving its
	// last-written content sitting there - Flush() draws a batch's entire cpuVertices range regardless of which
	// slots are still "in use", so without degenerating it below, the old region would keep rendering its last
	// real quads forever (e.g. a label's previous, shorter text ghosting behind its new longer text).
	uint32_t newQuadCapacity = (quadCount * 3 + 1) / 2;  // ceil(quadCount * 1.5), integer-only

	UIBatchSlot slot;
	slot.vertexOffset = (uint32_t)batch.cpuVertices.size();
	slot.quadCapacity = newQuadCapacity;
	slot.quadCount    = quadCount;

	batch.cpuVertices.resize(batch.cpuVertices.size() + (size_t)newQuadCapacity * 4);
	// Kept in lockstep with cpuVertices (one entry per quad, i.e. per 4 vertices) - default to the white
	// texture for the newly-appended region until a WriteX call actually tags it, which is harmless since
	// nothing indexes an unwritten quad until some WriteX gives it real vertex data too.
	batch.quadTextures.resize(batch.quadTextures.size() + newQuadCapacity, m_whiteTexture);
	batch.quadZIndex.resize(batch.quadZIndex.size() + newQuadCapacity, 0);
	batch.dirty = true;

	// existing.quadCapacity == 0 means this is a first-ever allocation (default-constructed UIBatchSlot), not a
	// relocation - nothing real was ever written there, so there's nothing to degenerate.
	if (existing.quadCapacity > 0)
	{
		ClearSlotFrom(batchHandle, existing, 0);
	}

	return slot;
}

void Struktur::Renderer::UIRenderer::Flush(GameContext& context)
{
	uint64_t drawState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	                     BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	// Active batches, ordered by drawOrder - separate from m_batches' own slot indices (see CreateBatch/
	// DestroyBatch), which are stable but otherwise unordered.
	std::vector<uint32_t> order;
	order.reserve(m_batches.size());
	for (uint32_t i = 0; i < (uint32_t)m_batches.size(); ++i)
	{
		if (m_batches[i].active)
		{
			order.push_back(i);
		}
	}
	std::sort(order.begin(), order.end(),
	          [this](uint32_t a, uint32_t b) { return m_batches[a].drawOrder < m_batches[b].drawOrder; });

	for (uint32_t index : order)
	{
		UIBatch& batch     = m_batches[index];
		uint32_t quadCount = (uint32_t)(batch.cpuVertices.size() / 4);
		if (quadCount == 0)
		{
			continue;
		}

		if (batch.dirty)
		{
			// Draw order within this one batch (see UIBatch::sortedQuadOrder) - z-index first, then the quad's
			// own raw index as a stable "order added to the batch" tiebreak for equal z-indices, since std::sort
			// isn't guaranteed stable and quad index already reflects original write/allocation order.
			batch.sortedQuadOrder.resize(quadCount);
			for (uint32_t quad = 0; quad < quadCount; ++quad)
			{
				batch.sortedQuadOrder[quad] = quad;
			}
			std::sort(batch.sortedQuadOrder.begin(), batch.sortedQuadOrder.end(),
			          [&batch](uint32_t a, uint32_t b)
			          {
				          if (batch.quadZIndex[a] != batch.quadZIndex[b])
				          {
					          return batch.quadZIndex[a] < batch.quadZIndex[b];
				          }
				          return a < b;
			          });

			// Index content is otherwise a fixed, deterministic pattern derived purely from quad count (the same
			// 0,1,2,0,2,3-per-quad layout WorldRenderer/SubmitTexturedQuad use) - not worth mirroring on
			// UIBatch itself alongside cpuVertices, so it's rebuilt here whenever an upload is actually needed.
			// Built by walking sortedQuadOrder rather than 0..quadCount directly - a quad's vertex data never
			// moves from wherever AllocateOrResizeSlot placed it, only which index-buffer position references it,
			// so base is still that quad's own real vertexOffset-derived index, just written at its sorted slot.
			std::vector<uint16_t> indices(quadCount * 6);
			for (uint32_t i = 0; i < quadCount; ++i)
			{
				uint16_t base    = (uint16_t)(batch.sortedQuadOrder[i] * 4);
				indices[i * 6 + 0] = base + 0;
				indices[i * 6 + 1] = base + 1;
				indices[i * 6 + 2] = base + 2;
				indices[i * 6 + 3] = base + 0;
				indices[i * 6 + 4] = base + 2;
				indices[i * 6 + 5] = base + 3;
			}

			bgfx::update(batch.vb, 0,
			             bgfx::copy(batch.cpuVertices.data(),
			                        (uint32_t)(batch.cpuVertices.size() * sizeof(QuadVertex))));
			bgfx::update(batch.ib, 0, bgfx::copy(indices.data(), (uint32_t)(indices.size() * sizeof(uint16_t))));
			batch.dirty = false;
		}

		// Walk quads in sortedQuadOrder (z-index order, not raw quad index - see above), submitting once per
		// contiguous same-texture run (see UIBatch::quadTextures) - mirrors WorldRenderer::Flush's own
		// texture-based run-splitting, except every run here draws from this one persistent vb/ib via offset+count
		// rather than a fresh transient buffer, since the vertex data itself was already uploaded above (if
		// dirty). This is what lets e.g. a panel's white-texture background/border and a label's font-atlas text
		// safely share one batch, each still drawing at its own z-index rather than in raw write order.
		uint32_t runStart              = 0;
		bgfx::TextureHandle runTexture = batch.quadTextures[batch.sortedQuadOrder[0]];
		for (uint32_t quad = 0; quad <= quadCount; ++quad)
		{
			bool boundary =
			    (quad == quadCount) || (batch.quadTextures[batch.sortedQuadOrder[quad]].idx != runTexture.idx);
			if (!boundary)
			{
				continue;
			}

			uint32_t runCount = quad - runStart;
			if (runCount > 0)
			{
				// The index buffer's values are absolute vertex indices (base = quad*4 across the whole batch, see
				// the rebuild above), not relative to this run. bgfx applies setVertexBuffer's startVertex by
				// rebasing which vertex each index resolves to (offsetting the buffer binding itself), so passing
				// runStart*4 here on top of already-absolute indices would double-offset every run after the
				// first - reading some other quad's vertex data (wrong position/UV) while still drawing it with
				// this run's texture. Always bind from vertex 0 and let the index values (via setIndexBuffer's
				// offset below) do the addressing.
				bgfx::setVertexBuffer(0, batch.vb, 0, quadCount * 4);
				bgfx::setIndexBuffer(batch.ib, runStart * 6, runCount * 6);
				bgfx::setTexture(0, m_texColorSampler, runTexture);
				if (batch.hasClip)
				{
					bgfx::setScissor((uint16_t)batch.clipRect.x, (uint16_t)batch.clipRect.y,
					                 (uint16_t)batch.clipRect.width, (uint16_t)batch.clipRect.height);
				}
				bgfx::setState(drawState);
				bgfx::submit(GraphicsDevice::UIViewId, GetEmbeddedProgram("sprite"));
			}

			runStart = quad;
			if (quad < quadCount)
			{
				runTexture = batch.quadTextures[batch.sortedQuadOrder[quad]];
			}
		}
	}
}

// --- Batch writes (phase 2 - see UIRenderer's class comment; still not wired into UIElement/UIPanel/UILabel) ---

Struktur::Renderer::UIBatch* Struktur::Renderer::UIRenderer::ResolveBatch(UIBatchHandle handle, const char* callerName)
{
	ASSERT_MSG(handle.IsValid() && handle.index < m_batches.size(), "%s: invalid batch handle (index %u)", callerName,
	           handle.index);
	if (!handle.IsValid() || handle.index >= m_batches.size())
	{
		return nullptr;
	}
	return &m_batches[handle.index];
}

bool Struktur::Renderer::UIRenderer::ValidateSlotCapacity(const UIBatch& batch, const UIBatchSlot& slot,
                                                          uint32_t quadsNeeded, const char* callerName)
{
	bool fits = slot.quadCapacity >= quadsNeeded &&
	            (uint64_t)slot.vertexOffset + (uint64_t)slot.quadCapacity * 4 <= batch.cpuVertices.size();
	ASSERT_MSG(fits, "%s: slot (offset %u, capacity %u) can't hold %u quads (batch has %u vertices)", callerName,
	           slot.vertexOffset, slot.quadCapacity, quadsNeeded, (uint32_t)batch.cpuVertices.size());
	return fits;
}

void Struktur::Renderer::UIRenderer::WriteRect(UIBatchHandle batchHandle, const UIBatchSlot& slot,
                                               const Util::Math::Rect& rect, const Util::Color& color, int32_t zIndex)
{
	UIBatch* batch = ResolveBatch(batchHandle, "WriteRect");
	if (!batch || !ValidateSlotCapacity(*batch, slot, 1, "WriteRect"))
	{
		return;
	}

	batch->quadTextures[slot.vertexOffset / 4] = m_whiteTexture;
	batch->quadZIndex[slot.vertexOffset / 4]   = zIndex;
	FillQuadVertices(&batch->cpuVertices[slot.vertexOffset], rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, 1.0f,
	                 1.0f, PackColor(color));
	batch->dirty = true;
}

void Struktur::Renderer::UIRenderer::WriteRectOutline(UIBatchHandle batchHandle, const UIBatchSlot& slot,
                                                      const Util::Math::Rect& rect, float thickness,
                                                      const Util::Color& color, int32_t zIndex)
{
	UIBatch* batch = ResolveBatch(batchHandle, "WriteRectOutline");
	if (!batch || !ValidateSlotCapacity(*batch, slot, 4, "WriteRectOutline"))
	{
		return;
	}

	uint32_t quadIndex = slot.vertexOffset / 4;
	for (uint32_t i = 0; i < 4; ++i)
	{
		batch->quadTextures[quadIndex + i] = m_whiteTexture;
		batch->quadZIndex[quadIndex + i]   = zIndex;
	}
	uint32_t abgr        = PackColor(color);
	QuadVertex* quadBase = &batch->cpuVertices[slot.vertexOffset];

	// Same top/bottom/left/right 4-bar decomposition as DrawRectOutline, just written into the batch instead of
	// submitted immediately.
	FillQuadVertices(quadBase + 0 * 4, rect.x, rect.y, rect.width, thickness, 0.0f, 0.0f, 1.0f, 1.0f, abgr);
	FillQuadVertices(quadBase + 1 * 4, rect.x, rect.y + rect.height - thickness, rect.width, thickness, 0.0f, 0.0f,
	                 1.0f, 1.0f, abgr);
	FillQuadVertices(quadBase + 2 * 4, rect.x, rect.y, thickness, rect.height, 0.0f, 0.0f, 1.0f, 1.0f, abgr);
	FillQuadVertices(quadBase + 3 * 4, rect.x + rect.width - thickness, rect.y, thickness, rect.height, 0.0f, 0.0f,
	                 1.0f, 1.0f, abgr);

	batch->dirty = true;
}

void Struktur::Renderer::UIRenderer::WriteTexturedRect(UIBatchHandle batchHandle, const UIBatchSlot& slot,
                                                       const Util::Math::Rect& rect, const TextureHandle& texture,
                                                       const Util::Color& tint, int32_t zIndex)
{
	UIBatch* batch = ResolveBatch(batchHandle, "WriteTexturedRect");
	if (!batch || !ValidateSlotCapacity(*batch, slot, 1, "WriteTexturedRect"))
	{
		return;
	}

	batch->quadTextures[slot.vertexOffset / 4] = bgfx::TextureHandle{(uint16_t)texture.id};
	batch->quadZIndex[slot.vertexOffset / 4]   = zIndex;
	FillQuadVertices(&batch->cpuVertices[slot.vertexOffset], rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, 1.0f,
	                 1.0f, PackColor(tint));
	batch->dirty = true;
}

uint32_t Struktur::Renderer::UIRenderer::WriteText(UIBatchHandle batchHandle, const UIBatchSlot& slot,
                                                   const Text::Font& font, const std::string& text,
                                                   const glm::vec2& position, float fontSize, const Util::Color& color,
                                                   int32_t zIndex)
{
	UIBatch* batch = ResolveBatch(batchHandle, "WriteText");
	if (!batch || !ValidateSlotCapacity(*batch, slot, 0, "WriteText"))
	{
		return 0;
	}

	// Stub/unloaded font (see FontResource) - write nothing rather than divide by zero below, matching DrawText.
	if (font.baseSize <= 0 || font.glyphs.empty())
	{
		return 0;
	}

	float scale               = fontSize / (float)font.baseSize;
	float atlasWidth          = (float)font.texture.width;
	float atlasHeight         = (float)font.texture.height;
	uint32_t abgr             = PackColor(color);
	bgfx::TextureHandle atlas = {(uint16_t)font.texture.id};

	uint32_t quadIndex    = slot.vertexOffset / 4;
	float penX            = position.x;
	int index             = 0;
	int size              = (int)text.size();
	uint32_t quadsWritten = 0;
	while (index < size)
	{
		int codepointByteCount = 0;
		int codepoint          = Text::GetCodepointNext(&text[index], &codepointByteCount);
		int glyphIndex         = Text::GetGlyphIndex(font, codepoint);
		index += codepointByteCount;

		const Text::Glyph& glyph    = font.glyphs[glyphIndex];
		const Util::Math::Rect& rec = glyph.rec;

		if (codepoint != ' ' && codepoint != '\t')
		{
			// See WriteText's header comment for the full contract - the caller must have already sized slot
			// to fit via AllocateOrResizeSlot. Stop rather than write past the end of cpuVertices if it didn't.
			ASSERT_MSG(quadsWritten < slot.quadCapacity,
			           "WriteText: slot capacity %u exceeded - caller must AllocateOrResizeSlot before WriteText "
			           "when text content grows",
			           slot.quadCapacity);
			if (quadsWritten >= slot.quadCapacity)
			{
				break;
			}

			float dstX                                    = penX + (float)glyph.offsetX * scale;
			float dstY                                    = position.y + (float)glyph.offsetY * scale;
			batch->quadTextures[quadIndex + quadsWritten] = atlas;
			batch->quadZIndex[quadIndex + quadsWritten]   = zIndex;
			FillQuadVertices(&batch->cpuVertices[slot.vertexOffset + quadsWritten * 4], dstX, dstY, rec.width * scale,
			                 rec.height * scale, rec.x / atlasWidth, rec.y / atlasHeight,
			                 (rec.x + rec.width) / atlasWidth, (rec.y + rec.height) / atlasHeight, abgr);
			++quadsWritten;
		}

		penX += (float)(glyph.advanceX > 0 ? glyph.advanceX : (int)rec.width) * scale;
	}

	batch->dirty = true;
	return quadsWritten;
}

void Struktur::Renderer::UIRenderer::ClearSlotFrom(UIBatchHandle batchHandle, const UIBatchSlot& slot,
                                                   uint32_t fromQuad)
{
	UIBatch* batch = ResolveBatch(batchHandle, "ClearSlotFrom");
	if (!batch || fromQuad >= slot.quadCapacity)
	{
		return;
	}

	uint32_t quadIndex = slot.vertexOffset / 4;
	for (uint32_t quad = fromQuad; quad < slot.quadCapacity; ++quad)
	{
		QuadVertex* v = &batch->cpuVertices[slot.vertexOffset + quad * 4];
		v[0] = v[1] = v[2] = v[3]             = QuadVertex{0.0f, 0.0f, 0.0f, 0.0f, 0};
		batch->quadTextures[quadIndex + quad] = m_whiteTexture;
		batch->quadZIndex[quadIndex + quad]   = 0;
	}
	batch->dirty = true;
}

// --- Batch assignment (phase 3 - see UIRenderer.h's class/method comment; still doesn't touch any Render()) ---

void Struktur::Renderer::UIRenderer::AssignBatches(UI::UIElement* element, UIBatchHandle sharedBatch)
{
	if (!element)
	{
		return;
	}

	UIBatchHandle effectiveBatch = sharedBatch;
	if (element->IsBatchRoot())
	{
		// Created once, then reused on every later AssignBatches pass over this element (rather than minting a
		// fresh batch - and orphaning the old one's GPU buffers - every time).
		if (!element->m_ownBatch.IsValid())
		{
			element->m_ownBatch = CreateBatch();
			// Initial sync only - SetZIndex on an already-assigned batch root must call SetBatchDrawOrder itself
			// (see WrenUI.cpp's setZIndex binding) to update this afterward, same as this class's other
			// "renderer-side state needs an explicit follow-up call" spots (see SetBatchRoot's own comment).
			m_batches[element->m_ownBatch.index].drawOrder = element->GetZIndex();
		}
		effectiveBatch = element->m_ownBatch;
	}

	// A slot's vertexOffset only means anything within the batch it was allocated from. If this element
	// previously belonged to a different batch (e.g. it was re-parented via AddChild into a subtree with a
	// different ambient/own batch than it had before), reusing its old m_batchSlot here would make
	// AllocateOrResizeSlot treat that stale offset as already-valid capacity in effectiveBatch and write
	// straight into whatever unrelated element actually occupies that position there. Degenerate its quads in
	// the old batch first (same reasoning as AllocateOrResizeSlot's own old-region cleanup) and force a fresh
	// allocation instead.
	if (element->m_batch.IsValid() && element->m_batch.index != effectiveBatch.index)
	{
		if (element->m_batchSlot.quadCapacity > 0)
		{
			ClearSlotFrom(element->m_batch, element->m_batchSlot, 0);
		}
		element->m_batchSlot = UIBatchSlot{};
	}

	element->m_batch     = effectiveBatch;
	element->m_batchSlot = AllocateOrResizeSlot(effectiveBatch, element->m_batchSlot, element->GetRequiredQuadCount());

	for (const auto& child : element->GetChildren())
	{
		AssignBatches(child.get(), effectiveBatch);
	}
}
