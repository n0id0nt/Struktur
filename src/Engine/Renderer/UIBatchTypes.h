#pragma once

#include <cstdint>

namespace Struktur
{
namespace Renderer
{
// Opaque reference to a slot in UIRenderer's internal batch pool (see UIRenderer::CreateBatch) - stays valid
// across other batches being created/destroyed, since slots are never erased/shifted, only reused via a
// free-list (see UIRenderer::m_freeBatchSlots). Split into its own lightweight header (no bgfx dependency) so
// UIElement.h can hold one by value without pulling bgfx types into every UI widget header.
struct UIBatchHandle
{
	uint32_t index = UINT32_MAX;
	bool IsValid() const
	{
		return index != UINT32_MAX;
	}
};

// Describes where a batch's already-written quads live within that batch's own cpuVertices. Callers must hold
// onto this (not just the UIBatchHandle) and replace their copy after every AllocateOrResizeSlot call, since a
// resize can relocate the offset (see UIRenderer::AllocateOrResizeSlot).
struct UIBatchSlot
{
	uint32_t vertexOffset = 0;
	uint32_t quadCapacity = 0;
	uint32_t quadCount    = 0;
};

// Opaque reference to a slot in UIRenderer's internal *animated* batch pool (see
// UIRenderer::CreateOrUpdateAnimatedBatch) - a separate handle space from UIBatchHandle above, since animated
// glyph quads (AnimQuadVertex) aren't QuadVertex-compatible and live in their own pool. Same
// never-erased-only-reused-via-free-list stability guarantee as UIBatchHandle.
struct AnimatedBatchHandle
{
	uint32_t index = UINT32_MAX;
	bool IsValid() const
	{
		return index != UINT32_MAX;
	}
};
}  // namespace Renderer
}  // namespace Struktur
