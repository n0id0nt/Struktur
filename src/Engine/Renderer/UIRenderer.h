#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Engine/Renderer/AnimQuadVertex.h"
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
// Border insets for UIRenderer::DrawNineSlice, in SOURCE TEXTURE pixel space - the four corners are sampled and
// drawn at this native pixel size regardless of the target rect's own size (never scaled), while the four edges
// stretch along one axis and the center stretches along both to fill whatever's left. Matches the conventional
// nine-slice/9-patch interpretation (Unity's Image "Sliced" mode, Godot's NinePatchRect, CSS border-image).
struct NineSliceBorder
{
	float left, right, top, bottom;
};

// One contiguous same-texture span within a batch's sortedQuadOrder (see UIBatch::runs) - start is implicit
// (0 for the first run, otherwise the previous run's own endQuad), since runs are always contiguous and
// gap-free by construction.
struct UIBatchRun
{
	uint32_t endQuad;
	bgfx::TextureHandle texture;
	// Whether this run's texture is a single-channel coverage atlas (font glyphs, see UIBatch::quadIsCoverage)
	// rather than a normal RGBA texture (backgrounds, borders, icons) - selects fs_spriteCoverage.sc instead of
	// fs_sprite.sc in Flush()'s submit loop. Safe to read off just the run's first quad: run-splitting already
	// groups strictly by texture identity, and a given bgfx texture handle is always either the font atlas or
	// not, so coverage-ness never changes mid-run.
	bool isCoverage;
};

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
	// One entry per quad, parallel to quadTextures - true if that quad samples a single-channel coverage atlas
	// (currently only DrawText's font atlas) and should be drawn with fs_spriteCoverage.sc instead of
	// fs_sprite.sc (see UIBatchRun::isCoverage and Flush()). Kept per-quad rather than per-texture-handle
	// because runs are already split by texture identity, so folding this into that same run-detection pass is
	// free - see Flush()'s run-building loop.
	std::vector<uint8_t> quadIsCoverage;
	// One entry per quad, parallel to quadTextures - the z-index of whichever element wrote that quad (see
	// DrawRect/DrawRectOutline/DrawTexturedRect/DrawText). Used only to order quads sharing this one batch
	// against each other (see sortedQuadOrder below); ordering BETWEEN batches is drawOrder's job, not this.
	std::vector<int32_t> quadZIndex;
	// Index-buffer build order, cached and only recomputed when dirty (see Flush()) - quad indices sorted by
	// (quadZIndex, quad index itself as the "added to the batch" tiebreak), so quads with equal z-index draw in
	// the order they were originally written rather than in some sort-unstable arbitrary order. Quad vertex data
	// never moves from wherever AllocateOrResizeSlot placed it - only the order this list visits quads in changes
	// what ends up in the index buffer, so a slot's vertexOffset stays a stable, reusable address regardless of
	// z-index.
	std::vector<uint32_t> sortedQuadOrder;
	// Cached, dirty-gated draw list (like ImGui's ImDrawList) - contiguous same-texture runs over sortedQuadOrder,
	// derived once alongside it (see Flush()) instead of every frame. Flush()'s per-frame submit loop just walks
	// this directly; the O(quadCount) work of re-detecting texture-run boundaries only happens when dirty.
	std::vector<UIBatchRun> runs;
	Util::Math::Rect clipRect;
	bool hasClip = false;
	// Cross-batch draw order only (each UIManager-level root element gets its own batch - see
	// UIRenderer::AssignBatches/UIManager::AddElement) - has no bearing on quad order within one batch, see
	// sortedQuadOrder above for that.
	int32_t drawOrder = 0;
	bool dirty        = true;
	bool active       = false;
};

// One animated-glyph batch always holds exactly one (texture, resolved effect-param) group's worth of quads -
// unlike UIBatch, there's no internal multi-texture run-splitting here (see AnimatedBatchHandle's own
// comment): UIRichLabel creates one of these per distinct animated group instead. Deliberately simpler than
// UIBatch since animated content is a handful of quads at most in realistic usage - see the Phase 8 design
// doc for why this trade was made.
struct AnimatedTextBatch
{
	bgfx::DynamicVertexBufferHandle vb = BGFX_INVALID_HANDLE;
	bgfx::DynamicIndexBufferHandle ib  = BGFX_INVALID_HANDLE;
	std::vector<AnimQuadVertex> cpuVertices;
	bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
	// Resolved uniform values for whichever effect bits the vertex data's packed animEffectMask actually turns
	// on - an inactive effect's fields are simply never read by the shader, so leaving them at 0 is harmless
	// (see vs_richTextAnim.sc).
	float waveAmplitude     = 0.0f;
	float waveFrequency     = 0.0f;
	float shakeRate         = 0.0f;
	float shakeLevel        = 0.0f;
	float pulseFrequency    = 0.0f;
	float rainbowFrequency  = 0.0f;
	float rainbowSaturation = 0.0f;
	float rainbowValue      = 0.0f;
	float tornadoRadius     = 0.0f;
	float tornadoFrequency  = 0.0f;
	float fadeStart         = 0.0f;
	float fadeLength        = 0.0f;
	// Which of GameContext::GetTimeSystem()'s two clocks drives the `time` uniform for this batch (see
	// SubmitAnimatedBatch) - per-batch rather than a single global choice so e.g. a main-menu label can animate
	// while the game itself is paused/time-scaled (scaledTime would freeze/slow with it) while gameplay rich
	// text still follows scaledTime as before. Set once by UIRichLabel from its own m_useUnscaledTime at group-
	// build time (see CreateOrUpdateAnimatedBatch's AnimationUniformValues parameter).
	bool useUnscaledTime = false;
	// Cross-batch draw order, same role/semantics as UIBatch::drawOrder - UIRenderer::Flush() merge-sorts both
	// handle spaces together by this so an animated batch interleaves correctly with regular UIBatches.
	int32_t drawOrder = 0;
	bool dirty        = true;
	bool active       = false;
};

// Effect parameter values a caller (UIRichLabel) resolves once per animated group and hands to
// CreateOrUpdateAnimatedBatch - a plain value type so the caller doesn't need to reach into
// AnimatedTextBatch's own fields directly.
struct AnimationUniformValues
{
	float waveAmplitude     = 0.0f;
	float waveFrequency     = 0.0f;
	float shakeRate         = 0.0f;
	float shakeLevel        = 0.0f;
	float pulseFrequency    = 0.0f;
	float rainbowFrequency  = 0.0f;
	float rainbowSaturation = 0.0f;
	float rainbowValue      = 0.0f;
	float tornadoRadius     = 0.0f;
	float tornadoFrequency  = 0.0f;
	float fadeStart         = 0.0f;
	float fadeLength        = 0.0f;
	// See AnimatedTextBatch::useUnscaledTime - copied straight through by CreateOrUpdateAnimatedBatch.
	bool useUnscaledTime = false;
};

// bgfx-native replacement for raylib's DrawRectangleRec/DrawRectangleLinesEx/DrawTexturePro/DrawTextEx on the
// desktop UI path (UIElement/UIPanel/UILabel) - the UI-rendering equivalent of ImGuiRenderer. Owns a lazily-
// created 1x1 white texture so solid-color quads reuse the same textured-quad submission (and therefore the
// same "sprite" program) as panel backgrounds and glyphs, matching fs_sprite.sc's
// texture.rgba * vertexColor.rgba blend model.
//
// All UI rendering goes through the persistent-buffer batching path below (CreateBatch/DestroyBatch/
// AllocateOrResizeSlot/DrawRect/DrawRectOutline/DrawTexturedRect/DrawText/AssignBatches/Flush) - there is no
// immediate-mode fallback (that used to exist here as a separate Draw*/SubmitTexturedQuad pair alongside these,
// each glyph or rect submitted as its own bgfx draw call; removed once every caller had a real batch/slot to
// write into instead, see UIManager's own focus-indicator batch and Game.cpp's splash screen for the two
// non-UIElement callers that needed one added).
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
	// real window) - call once per frame before any batch write below (see UIRenderSystem::Update).
	void SetupView(GameContext& context);

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
	// Cross-batch draw order (see UIBatch::drawOrder) - callers must call this again themselves whenever a batch
	// root's z-index changes after the fact (see WrenUI.cpp's setZIndex binding), same "renderer-side state needs
	// an explicit follow-up call" contract SetBatchRoot's own comment already documents for AssignBatches.
	void SetBatchDrawOrder(UIBatchHandle handle, int32_t drawOrder);
	// Sets the batch's scissor rect (see UIBatch::clipRect/hasClip, already consumed by SubmitBatch's
	// bgfx::setScissor) - clipping is strictly per-batch, applied uniformly to every run within it, so a subtree
	// that needs to be clipped independent of whatever it's nested inside must be its own batch root (see
	// UIClip). Safe to call on any batch regardless of prior clip state; bgfx resets scissor to none after every
	// submit() (BGFX_DISCARD_ALL, this codebase's default), so there's no bleed risk between batches even without
	// an explicit Clear.
	void SetBatchClip(UIBatchHandle handle, const Util::Math::Rect& clipRect);
	void ClearBatchClip(UIBatchHandle handle);
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

	// --- Batch writes - fill a batch's cpuVertices at a caller-owned slot rather than submitting immediately
	// (wired into UIPanel/UILabel's Render(), see UIElement's class comment, plus UIManager's focus indicator and
	// Game.cpp's splash screen for the two non-UIElement callers). Every DrawX function below sets
	// batch.dirty = true and tags the quads it just wrote with whatever texture it used (UIBatch::quadTextures) -
	// a batch is not limited to one texture (see Flush()), so writes from differently-textured widgets sharing a
	// batch don't stomp on each other.
	// zIndex tags the quad(s) written for later ordering against other quads sharing this batch (see
	// UIBatch::quadZIndex/sortedQuadOrder and Flush()) - callers pass their own GetZIndex().
	void DrawRect(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect, const Util::Color& color,
	              int32_t zIndex);
	// Writes 4 quads (top/bottom/left/right bars) - slot must have quadCapacity >= 4.
	void DrawRectOutline(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect, float thickness,
	                     const Util::Color& color, int32_t zIndex);
	void DrawTexturedRect(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect,
	                      const TextureHandle& texture, const Util::Color& tint, int32_t zIndex);
	// Same as DrawTexturedRect, but samples sourceRectPixels (a pixel-space sub-rect of texture, converted to
	// UV internally via texture's own width/height) instead of the whole 0..1 UV range - for atlas-style
	// textures where many sprites/icons share one texture (see UI::IconAtlas). Still a single quad, still
	// tagged quadIsCoverage=false (full RGBA, tinted by `tint` in fs_sprite.sc) - no shader/batching changes
	// needed versus DrawTexturedRect, only the UV math differs.
	void DrawTexturedRectRegion(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect,
	                            const TextureHandle& texture, const Util::Math::Rect& sourceRectPixels,
	                            const Util::Color& tint, int32_t zIndex);
	// Writes 9 quads (4 fixed-size corners, 4 stretched edges, 1 stretched center - see NineSliceBorder) - slot
	// must have quadCapacity >= 9. `border` is clamped internally so opposing edges never overlap when rect is
	// smaller than the border insets (the sampled source region itself is never distorted by that clamp, only
	// the on-screen size), matching the defensive handling other engines' nine-slice implementations apply at
	// very small sizes.
	void DrawNineSlice(UIBatchHandle batch, const UIBatchSlot& slot, const Util::Math::Rect& rect,
	                   const TextureHandle& texture, const NineSliceBorder& border, const Util::Color& tint,
	                   int32_t zIndex);
	// Walks `text` as UTF-8 via Text::GetCodepointNext/Text::GetGlyphIndex, writing into batch's cpuVertices at
	// slot - one quad per glyph, skipping space/tab - so the number of quads written can be less than text's
	// codepoint count. Single line only (callers already split multi-line text, see UILabel::GetTextLines).
	// Returns the quad count actually written.
	//
	// CONTRACT: slot.quadCapacity must already be large enough for text's worst case (its codepoint count is a
	// safe upper bound, since space/tab codepoints don't emit a quad) - this function does NOT grow the slot
	// itself. If capacity runs out mid-string, it ASSERT_MSGs and stops (returning however many quads it got to
	// rather than writing past the end of cpuVertices). Callers (UILabel) are responsible for calling
	// AllocateOrResizeSlot first whenever text content changes and may have grown past the slot's current
	// capacity - this function does not, and cannot, know whether that happened.
	//
	// shearAmount (default 0, every existing caller unaffected) skews every glyph quad into a parallelogram -
	// see FillQuadVertices in UIRenderer.cpp. Exists for UIRichLabel's synthetic-italic fallback (see
	// UIRichLabel::ResolveFont): a run that needs italics but has no dedicated italic FontResource set still
	// gets a plausible slant instead of silently rendering upright.
	uint32_t DrawText(UIBatchHandle batch, const UIBatchSlot& slot, const Text::Font& font, const std::string& text,
	                  const glm::vec2& position, float fontSize, const Util::Color& color, int32_t zIndex,
	                  float shearAmount = 0.0f);

	// Degenerates (zero-area, so Flush() draws nothing for them) every quad in slot from fromQuad up to
	// slot.quadCapacity. Flush() draws a batch's whole allocated cpuVertices region regardless of how many quads
	// a given widget's writes actually touched this frame - so when a variable-quad-count write (DrawText, when
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

	// --- Animated text batch storage (see AnimatedTextBatch's own comment) - a separate small subsystem from
	// the UIBatch path above, not an extension of it. ---

	// Creates a fresh animated batch (existing invalid) or updates an existing one's content/params/texture in
	// place (existing valid, reusing its vb/ib exactly like AllocateOrResizeSlot's grow-in-place story - the
	// underlying BGFX_BUFFER_ALLOW_RESIZE buffers already support this). Unlike the regular UIBatch path,
	// there's no separate "allocate slot, then write into it" split - callers always hand over the group's
	// complete vertex data in one call, since animated content changes shape rarely enough that rebuilding it
	// wholesale each time is simpler than incremental slot management for a second vertex format.
	AnimatedBatchHandle CreateOrUpdateAnimatedBatch(AnimatedBatchHandle existing,
	                                                const std::vector<AnimQuadVertex>& vertices,
	                                                bgfx::TextureHandle texture, const AnimationUniformValues& params,
	                                                int32_t drawOrder);
	void DestroyAnimatedBatch(AnimatedBatchHandle handle);

private:
	// Submits one already-uploaded UIBatch's cached runs (see UIBatch::runs) - the body of Flush()'s original
	// per-batch loop, extracted so Flush() can merge-sort regular and animated batches together by drawOrder
	// and dispatch to whichever submit helper each item needs.
	void SubmitBatch(UIBatch& batch, uint64_t drawState);
	// Submits one animated batch - re-uploads its vertex/index data only if dirty (same as SubmitBatch), but
	// always (re)binds the current time + per-effect-type uniforms before every submit regardless of dirty,
	// since that's what actually animates the already-uploaded vertex data frame to frame (see the Phase 8
	// design doc - this is the "zero per-frame CPU rebuild" property the shader-based approach was chosen for).
	void SubmitAnimatedBatch(GameContext& context, AnimatedTextBatch& batch, uint64_t drawState);

	// Resolves handle to its UIBatch for a DrawX call, ASSERT_MSGing (see Debug/Assertions.h) on an invalid
	// handle - returns nullptr on failure so callers can bail out safely even in Release, where the assert
	// itself is a no-op. callerName is folded into the assert message so it names the actual DrawX function
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

	// Same stability/free-list-reuse story as m_batches/m_freeBatchSlots above, just for AnimatedTextBatch's
	// separate handle space (see AnimatedBatchHandle).
	std::vector<AnimatedTextBatch> m_animBatches;
	std::vector<uint32_t> m_freeAnimBatchSlots;
};
}  // namespace Renderer
}  // namespace Struktur
