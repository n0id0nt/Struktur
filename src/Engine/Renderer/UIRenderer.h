#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Engine/Renderer/QuadVertex.h"
#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Renderer/UIBatchTypes.h"
#include "Engine/Text/Font.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"

namespace Struktur
{
class GameContext;

namespace UI
{
class UIElement;
}
}  // namespace Struktur

namespace Struktur
{
namespace Renderer
{
// One persistent, update-in-place bgfx buffer pair plus its CPU-side vertex mirror. Unlike WorldRenderer's
// transient per-frame sprite batches, UI content (panels/labels) mostly doesn't change frame to frame, so this
// is only re-uploaded to the GPU when actually dirtied (see UIRenderer::Flush), not rebuilt every frame.
struct UIBatch
{
	bgfx::DynamicVertexBufferHandle vb = BGFX_INVALID_HANDLE;
	bgfx::DynamicIndexBufferHandle ib  = BGFX_INVALID_HANDLE;
	std::vector<QuadVertex> cpuVertices;
	// One entry per quad (always kept the same length as cpuVertices.size()/4 - see AllocateOrResizeSlot) -
	// which texture that specific quad samples from. A batch is NOT limited to one texture: Flush() walks
	// quads in order and splits into a separate bgfx::submit() per contiguous same-texture run (mirroring
	// WorldRenderer::Flush's own texture-based run-splitting), all against this one persistent vb/ib via
	// offset+count, not a fresh buffer per run. This is what lets differently-textured widgets (e.g. a panel's
	// solid-color background next to a label's font atlas) safely share one batch/vertex cache.
	std::vector<bgfx::TextureHandle> quadTextures;
	Util::Math::Rect clipRect;
	bool hasClip       = false;
	uint32_t drawOrder = 0;
	bool dirty         = true;
	bool active        = false;
};

// bgfx-native replacement for raylib's DrawRectangleRec/DrawRectangleLinesEx/DrawTexturePro/DrawTextEx on the
// desktop UI path (UIElement/UIPanel/UILabel) - the UI-rendering equivalent of ImGuiRenderer. Owns a lazily-
// created 1x1 white texture so solid-color quads reuse the same textured-quad submission (and therefore the
// same "sprite" program) as panel backgrounds and glyphs, matching fs_sprite.sc's
// texture.rgba * vertexColor.rgba blend model.
//
// Also owns the persistent-buffer batching path (CreateBatch/DestroyBatch/AllocateOrResizeSlot/WriteRect/
// WriteRectOutline/WriteTexturedRect/WriteText/AssignBatches/Flush below) that UIPanel/UILabel's Render() write
// into instead of drawing immediately, once m_batch/m_batchSlot have been assigned (see UIElement's own class
// comment and UIRenderSystem for how AssignBatches/Flush get called). DrawRect/DrawRectOutline/DrawTexturedRect/
// DrawText/SubmitTexturedQuad remain as the immediate-mode path for transient/rare draws that don't benefit from
// a persistent slot - RenderFocusIndicator and debug/overlay drawing use these directly.
class UIRenderer
{
public:
	UIRenderer()  = default;
	~UIRenderer() = default;

	UIRenderer(const UIRenderer&)            = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	// Actually creates the white texture/sampler uniform - deferred from construction so GameContext can own
	// this unconditionally (see GameContext's constructor) while still requiring bgfx to already exist by the
	// time this runs (call right after GraphicsDevice::Initialise).
	void Initialise();
	// Explicit rather than in the destructor - must run before GraphicsDevice::Shutdown() (see
	// GameContext::Shutdown()), since bgfx::destroy() is invalid once bgfx itself has shut down.
	void Shutdown();

	// Sets GraphicsDevice::UIViewId's per-frame orthographic transform, sized to the game viewport (not the
	// real window) - call once per frame before any Draw* call (see UIRenderSystem::Update).
	void SetupView(GameContext& context);

	void DrawRect(const Util::Math::Rect& rect, const Util::Color& color);
	void DrawRectOutline(const Util::Math::Rect& rect, float thickness, const Util::Color& color);
	void DrawTexturedRect(const Util::Math::Rect& rect, const TextureHandle& texture, const Util::Color& tint);
	// Walks `text` as UTF-8 via Text::GetCodepointNext/Text::GetGlyphIndex and emits one quad per glyph from
	// font.glyphs - single line only (callers already split multi-line text, see UILabel::GetTextLines).
	void DrawText(const Text::Font& font, const std::string& text, const glm::vec2& position, float fontSize,
	              const Util::Color& color);

	// --- Batch storage (see class comment above) ---

	// Allocates a batch slot (reusing a freed one if one exists) and creates its bgfx dynamic vertex/index
	// buffers at an initial capacity of kInitialBatchQuadCapacity quads, with BGFX_BUFFER_ALLOW_RESIZE so later
	// AllocateOrResizeSlot growth can actually take effect on the GPU buffer. The returned handle stays valid
	// until the matching DestroyBatch.
	UIBatchHandle CreateBatch();
	// Destroys the batch's bgfx buffers, resets its slot to a fresh inactive UIBatch, and pushes the slot onto
	// the free-list for a future CreateBatch to reuse. Asserts (see Debug/Assertions.h's ASSERT_MSG) on an
	// invalid handle or a double-destroy, but still bails out safely afterward rather than trusting the handle.
	void DestroyBatch(UIBatchHandle handle);
	// Ensures batch's cpuVertices can hold quadCount quads, reusing existing's slot in place if it's already
	// big enough (only quadCount changes), or relocating to a freshly appended region at the end of
	// cpuVertices sized to ceil(quadCount * 1.5) quads otherwise - growing in place isn't attempted, so the old
	// region at existing.vertexOffset is simply abandoned. Callers MUST overwrite their stored UIBatchSlot with
	// the one returned here, since a relocation changes the offset.
	UIBatchSlot AllocateOrResizeSlot(UIBatchHandle batch, UIBatchSlot existing, uint32_t quadCount);
	// Uploads any dirty batch's cpuVertices (and its derived index pattern - see Flush()'s own comment) to its
	// GPU buffers, then submits every active batch in drawOrder - as one or more bgfx draw calls per batch,
	// split by contiguous same-texture runs of quads (see UIBatch::quadTextures). Wired into the frame loop via
	// UIRenderSystem, once per frame after all widget Render() calls.
	void Flush(GameContext& context);

	// --- Batch writes (phase 2 - fill a batch's cpuVertices at a caller-owned slot instead of submitting
	// immediately; wired into UIPanel/UILabel's Render() as of phase 4, see UIElement's class comment). Every
	// WriteX function below sets batch.dirty = true and tags the quads it just wrote with whatever texture it
	// used (UIBatch::quadTextures) - a batch is not limited to one texture (see Flush()), so writes from
	// differently-textured widgets sharing a batch don't stomp on each other.
	void WriteRect(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect,
	               const Util::Color& color);
	// Writes 4 quads (top/bottom/left/right bars, the same decomposition DrawRectOutline uses) - slot must have
	// quadCapacity >= 4.
	void WriteRectOutline(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect, float thickness,
	                      const Util::Color& color);
	void WriteTexturedRect(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect,
	                       const TextureHandle& texture, const Util::Color& tint);
	// Same UTF-8 glyph-walk as DrawText (Text::GetCodepointNext/Text::GetGlyphIndex), writing into batch's
	// cpuVertices at slot instead of submitting immediately, and skipping a quad for space/tab exactly like
	// DrawText does - so the number of quads written can be less than text's codepoint count. Returns the quad
	// count actually written.
	//
	// CONTRACT: slot.quadCapacity must already be large enough for text's worst case (its codepoint count is a
	// safe upper bound, since space/tab codepoints don't emit a quad) - this function does NOT grow the slot
	// itself. If capacity runs out mid-string, it ASSERT_MSGs and stops (returning however many quads it got to
	// rather than writing past the end of cpuVertices). Callers (UILabel) are responsible for calling
	// AllocateOrResizeSlot first whenever text content changes and may have grown past the slot's current
	// capacity - this function does not, and cannot, know whether that happened.
	uint32_t WriteText(UIBatchHandle batch, const UIBatchSlot& slot, const Text::Font& font, const std::string& text,
	                   const glm::vec2& position, float fontSize, const Util::Color& color);

	// Degenerates (zero-area, so Flush() draws nothing for them) every quad in slot from fromQuad up to
	// slot.quadCapacity. Flush() draws a batch's whole allocated cpuVertices region regardless of how many quads
	// a given widget's writes actually touched this frame - so when a variable-quad-count write (WriteText, when
	// text gets shorter) uses fewer quads than the slot's capacity, whatever real vertex data those trailing
	// quads held from a previous, longer write would otherwise keep being drawn as stale leftover geometry.
	// Callers that write a variable number of quads into a fixed-capacity slot (UILabel's text) must call this
	// after their last write, passing however many quads they actually used.
	void ClearSlotFrom(UIBatchHandle batch, const UIBatchSlot& slot, uint32_t fromQuad);

	// --- Batch assignment - walks a UIElement tree assigning each element to a batch and sizing its slot. ---
	//
	// Recursively, for `element` and then every descendant: resolves the effective batch (sharedBatch, unless
	// element->IsBatchRoot() - in which case element->m_ownBatch is created on first use and reused after,
	// becoming both element's own batch AND the batch shared with its subtree), sets element->m_batch to that,
	// sizes element->m_batchSlot via AllocateOrResizeSlot for element->GetRequiredQuadCount() quads, then
	// recurses into children with the resolved batch as their shared one. UIRenderer is a friend of UIElement
	// (see UIElement.h) so this can write m_batch/m_ownBatch/m_batchSlot directly rather than needing public
	// setters that any other caller could also reach for.
	void AssignBatches(UI::UIElement* element, UIBatchHandle sharedBatch);

private:
	void SubmitTexturedQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, uint32_t abgr,
	                        bgfx::TextureHandle texture);

	// Resolves handle to its UIBatch for a WriteX call, ASSERT_MSGing (see Debug/Assertions.h) on an invalid
	// handle - returns nullptr on failure so callers can bail out safely even in Release, where the assert
	// itself is a no-op. callerName is folded into the assert message so it names the actual WriteX function
	// that failed, not this shared helper.
	UIBatch* ResolveBatch(UIBatchHandle handle, const char* callerName);
	// Returns true if slot can safely hold quadsNeeded quads inside batch's cpuVertices (checks both declared
	// capacity and that [vertexOffset, vertexOffset + quadCapacity*4) is actually within cpuVertices' bounds, in
	// case a stale/wrong slot was passed in) - ASSERT_MSGs and returns false otherwise, the same "assert then
	// let the caller bail" pattern as ResolveBatch above.
	bool ValidateSlotCapacity(const UIBatch& batch, const UIBatchSlot& slot, uint32_t quadsNeeded,
	                          const char* callerName);

	bgfx::TextureHandle m_whiteTexture    = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;

	// Initial bgfx dynamic buffer size for a freshly created batch, in quads - grown as needed by
	// AllocateOrResizeSlot (see BGFX_BUFFER_ALLOW_RESIZE on the buffers CreateBatch creates).
	static constexpr uint32_t kInitialBatchQuadCapacity = 64;

	// Slots are never erased/shifted (see CreateBatch/DestroyBatch), so a UIBatchHandle from one batch stays
	// valid across another batch's create/destroy churn - freed indices are recycled via m_freeBatchSlots
	// instead of the vector ever resizing down.
	std::vector<UIBatch> m_batches;
	std::vector<uint32_t> m_freeBatchSlots;
};
}  // namespace Renderer
}  // namespace Struktur
