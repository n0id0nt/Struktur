#include "WorldRenderer.h"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <cfloat>
#include <glm/gtc/type_ptr.hpp>

#include "Engine/Core/GameData.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/EmbeddedShaders.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/QuadVertex.h"

Struktur::Renderer::WorldRenderer::WorldRenderer()
{
	m_drawItems.reserve(2048);
}

void Struktur::Renderer::WorldRenderer::Shutdown()
{
	if (bgfx::isValid(m_texColorSampler))
	{
		bgfx::destroy(m_texColorSampler);
		m_texColorSampler = BGFX_INVALID_HANDLE;
	}
}

Struktur::Renderer::CullBounds Struktur::Renderer::WorldRenderer::ComputeCullBounds(GameContext& context)
{
	World::Camera& camera    = context.GetCamera();
	Core::GameData& gameData = context.GetGameData();

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 inv  = glm::inverse(view);

	// Transform all 4 screen corners to world space (rather than 2) so a rotated camera still
	// produces a correct, if slightly conservative, axis-aligned world-space bound to cull against.
	glm::vec4 screenCorners[4] = {
	    {0.0f, 0.0f, 0.0f, 1.0f},
	    {(float)gameData.gameWidth, 0.0f, 0.0f, 1.0f},
	    {0.0f, (float)gameData.gameHeight, 0.0f, 1.0f},
	    {(float)gameData.gameWidth, (float)gameData.gameHeight, 0.0f, 1.0f},
	};

	CullBounds bounds{FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};
	for (const auto& screenCorner : screenCorners)
	{
		glm::vec4 worldCorner = inv * screenCorner;
		bounds.minX           = std::min(bounds.minX, worldCorner.x);
		bounds.minY           = std::min(bounds.minY, worldCorner.y);
		bounds.maxX           = std::max(bounds.maxX, worldCorner.x);
		bounds.maxY           = std::max(bounds.maxY, worldCorner.y);
	}
	return bounds;
}

uint64_t Struktur::Renderer::WorldRenderer::PackSortKey(World::RenderLayer layer, float orderInLayer,
                                                        unsigned int textureId, uint16_t programId)
{
	// [63:60] layer (4 bits) | [59:28] orderInLayer, sortable-float-bits (32 bits) | [27:12] textureId (16 bits -
	// matches bgfx::TextureHandle::idx's own uint16_t width) | [11:0] programId (12 bits - bgfx caps
	// BGFX_CONFIG_MAX_PROGRAMS at 512, so this is already 8x headroom). Folding program in here (rather than
	// only checking it as a run-boundary condition after sorting, like before) means the sort itself groups
	// same-texture-same-program items together, instead of them fragmenting into extra runs whenever items
	// with the same texture but different programs happen to interleave in submission order.
	uint64_t layerBits   = static_cast<uint64_t>(layer) & 0xFull;
	uint64_t orderBits   = static_cast<uint64_t>(Util::Math::FloatToSortableBits(orderInLayer));
	uint64_t textureBits = static_cast<uint64_t>(textureId) & 0xFFFFull;
	uint64_t programBits = static_cast<uint64_t>(programId) & 0xFFFull;
	return (layerBits << 60) | (orderBits << 28) | (textureBits << 12) | programBits;
}

void Struktur::Renderer::WorldRenderer::Clear()
{
	m_drawItems.clear();
}

namespace
{
// Fills the 4 corners of a rotated quad exactly like raylib's DrawTexturePro: destRec.x/y is the rotation
// pivot, origin is the (dest-space) offset from the pivot to the pre-rotation top-left corner.
void ComputeQuadCorners(const Struktur::Util::Math::Rect& destRec, const glm::vec2& origin, float rotationDegrees,
                        glm::vec2 outCorners[4])
{
	float rad = glm::radians(rotationDegrees);
	float cs  = std::cos(rad);
	float sn  = std::sin(rad);
	float dx  = -origin.x;
	float dy  = -origin.y;

	outCorners[0] = {destRec.x + dx * cs - dy * sn, destRec.y + dx * sn + dy * cs};
	outCorners[1] = {destRec.x + (dx + destRec.width) * cs - dy * sn, destRec.y + (dx + destRec.width) * sn + dy * cs};
	outCorners[2] = {destRec.x + dx * cs - (dy + destRec.height) * sn,
	                 destRec.y + dx * sn + (dy + destRec.height) * cs};
	outCorners[3] = {destRec.x + (dx + destRec.width) * cs - (dy + destRec.height) * sn,
	                 destRec.y + (dx + destRec.width) * sn + (dy + destRec.height) * cs};
}

// Computes the axis-aligned world-space bounding box actually covered by a sprite's quad, for culling against.
// destRec.x/y is the rotation pivot, not the quad's top-left corner (see ComputeQuadCorners above) - culling
// against destRec directly is wrong whenever origin is non-zero (e.g. a centered origin), since the real
// on-screen quad is shifted away from destRec's own naive rect. Rotation == 0 skips the full corner computation
// and 4-point min/max reduction below: an unrotated quad's AABB is just destRec translated by -origin, which is
// both cheaper and the overwhelmingly common case (most sprites don't rotate).
Struktur::Util::Math::Rect ComputeQuadWorldBounds(const Struktur::Util::Math::Rect& destRec, const glm::vec2& origin,
                                                  float rotationDegrees)
{
	if (rotationDegrees == 0.0f)
	{
		return {destRec.x - origin.x, destRec.y - origin.y, destRec.width, destRec.height};
	}

	glm::vec2 corners[4];
	ComputeQuadCorners(destRec, origin, rotationDegrees, corners);

	float minX = std::min({corners[0].x, corners[1].x, corners[2].x, corners[3].x});
	float minY = std::min({corners[0].y, corners[1].y, corners[2].y, corners[3].y});
	float maxX = std::max({corners[0].x, corners[1].x, corners[2].x, corners[3].x});
	float maxY = std::max({corners[0].y, corners[1].y, corners[2].y, corners[3].y});
	return {minX, minY, maxX - minX, maxY - minY};
}
}  // namespace

void Struktur::Renderer::WorldRenderer::SubmitSprite(World::RenderLayer layer, float orderInLayer,
                                                     bgfx::ProgramHandle program, const Component::Shader* shader,
                                                     const TextureHandle& texture, const Util::Math::Rect& sourceRec,
                                                     const Util::Math::Rect& destRec, const glm::vec2& origin,
                                                     float rotation, const Util::Color& tint,
                                                     const CullBounds& cullBounds)
{
	Util::Math::Rect worldBounds = ComputeQuadWorldBounds(destRec, origin, rotation);
	if (!Util::Math::RectOverlaps(worldBounds, cullBounds.minX, cullBounds.minY, cullBounds.maxX, cullBounds.maxY))
	{
		return;
	}

	DrawItem item{PackSortKey(layer, orderInLayer, texture.id, program.idx),
	              texture,
	              program,
	              shader,
	              sourceRec,
	              destRec,
	              origin,
	              rotation,
	              tint};
	m_drawItems.push_back(item);
}

void Struktur::Renderer::WorldRenderer::SubmitChunk(World::RenderLayer layer, float orderInLayer,
                                                    bgfx::ProgramHandle program, const Component::Shader* shader,
                                                    const TileChunk& chunk, const TextureHandle& texture,
                                                    const CullBounds& cullBounds)
{
	if (!Util::Math::RectOverlaps(chunk.worldBounds, cullBounds.minX, cullBounds.minY, cullBounds.maxX,
	                              cullBounds.maxY))
	{
		return;
	}

	DrawItem item{PackSortKey(layer, orderInLayer, texture.id, program.idx),
	              texture,
	              program,
	              shader,
	              {},
	              {},
	              {},
	              0.0f,
	              {},
	              &chunk};
	m_drawItems.push_back(item);
}

void Struktur::Renderer::WorldRenderer::Flush(GameContext& context)
{
	// stable_sort, not sort: two items with identical (layer, orderInLayer, texture, program) produce identical
	// sortKeys, and plain sort gives no ordering guarantee among ties - for overlapping same-order transparent
	// sprites that meant visible frame-to-frame flicker as entt's iteration order shifted. stable_sort makes
	// submission order the tiebreaker instead, which is deterministic. This is a real perf tradeoff (stable_sort
	// is typically O(n log n) with extra allocation/merge overhead vs. sort's in-place O(n log n)), but
	// deterministic paint order matters more here than the constant-factor cost at this list's size.
	std::stable_sort(m_drawItems.begin(), m_drawItems.end(),
	                 [](const DrawItem& a, const DrawItem& b) { return a.sortKey < b.sortKey; });

	System::ShaderSystem& shaderSystem = context.GetSystemManager().GetSystem<System::ShaderSystem>();
	World::Camera& camera              = context.GetCamera();
	Core::GameData& gameData           = context.GetGameData();

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = camera.GetProjectionMatrix(gameData.gameWidth, gameData.gameHeight);
	bgfx::setViewTransform(GraphicsDevice::WorldViewId, glm::value_ptr(view), glm::value_ptr(proj));

	static const bgfx::VertexLayout quadLayout = BuildQuadVertexLayout();
	if (!bgfx::isValid(m_texColorSampler))
	{
		m_texColorSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	}
	const bgfx::UniformHandle& texColorSampler = m_texColorSampler;

	uint64_t drawState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	                     BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	// The sort key already puts same-texture-same-program items adjacent (see PackSortKey); this walks the
	// list once, closing the current batch whenever the texture or program changes (both already resolved at
	// submit time - see DrawItem::program) and emitting exactly one bgfx draw call per contiguous run instead
	// of one per sprite. A chunk item always closes the run around it and submits its own cached buffer
	// directly - it's already a pre-built batch, so there's nothing to accumulate. A run is also force-closed
	// once it hits kMaxQuadsPerRun, even without a texture/program change, since FlushRun's 16-bit index
	// buffer would otherwise silently wrap and corrupt geometry past that many quads.
	size_t runStart                = 0;
	bgfx::ProgramHandle runProgram = BGFX_INVALID_HANDLE;
	Renderer::TextureHandle runTexture{0, 0, 0};

	for (size_t i = 0; i < m_drawItems.size(); ++i)
	{
		const DrawItem& item = m_drawItems[i];

		if (item.chunk)
		{
			FlushRun(context, runStart, i, quadLayout, texColorSampler, drawState, runTexture, runProgram);
			// Applied immediately before this chunk's own submit, not any earlier - see DrawItem::shader.
			if (item.shader)
			{
				shaderSystem.ApplyUniforms(context, *item.shader);
			}
			bgfx::setVertexBuffer(0, item.chunk->vb);
			bgfx::setIndexBuffer(item.chunk->ib);
			bgfx::setTexture(0, texColorSampler, {(uint16_t)item.texture.id});
			bgfx::setState(drawState);
			bgfx::submit(GraphicsDevice::WorldViewId, item.program);
			runStart = i + 1;
			continue;
		}

		bool boundary = (i == runStart) ? false
		                                : (item.texture.id != runTexture.id || item.program.idx != runProgram.idx ||
		                                   (i - runStart) >= kMaxQuadsPerRun);
		if (boundary)
		{
			FlushRun(context, runStart, i, quadLayout, texColorSampler, drawState, runTexture, runProgram);
			runStart = i;
		}
		runTexture = item.texture;
		runProgram = item.program;
	}
	FlushRun(context, runStart, m_drawItems.size(), quadLayout, texColorSampler, drawState, runTexture, runProgram);
}

void Struktur::Renderer::WorldRenderer::FlushRun(GameContext& context, size_t runStart, size_t runEnd,
                                                 const bgfx::VertexLayout& quadLayout,
                                                 bgfx::UniformHandle texColorSampler, uint64_t drawState,
                                                 const TextureHandle& runTexture, bgfx::ProgramHandle runProgram)
{
	size_t count = runEnd - runStart;
	if (count == 0)
	{
		return;
	}

	// Every item in a run shares the same program (that's what makes it a run - see Flush()'s boundary check),
	// but not necessarily the same Component::Shader instance, so only the first item's custom uniform values
	// actually take effect for the whole batched draw call - a pre-existing limit of merging multiple sprites
	// into one bgfx submit, not something fixable here without per-instance uniform buffers. Applied here,
	// immediately before this run's own submit below, rather than at SubmitSprite/SubmitChunk time - bgfx's
	// per-draw uniform state is only valid for the very next submit on this encoder, so setting it any earlier
	// (e.g. back when each item was first collected) meant a second shaded item's ApplyUniforms would try to set
	// the same uniform again before either had actually been submitted, which bgfx flags as an error.
	const Component::Shader* runShader = m_drawItems[runStart].shader;
	if (runShader)
	{
		context.GetSystemManager().GetSystem<System::ShaderSystem>().ApplyUniforms(context, *runShader);
	}

	bgfx::TransientVertexBuffer tvb;
	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientVertexBuffer(&tvb, (uint32_t)(count * 4), quadLayout);
	bgfx::allocTransientIndexBuffer(&tib, (uint32_t)(count * 6));

	QuadVertex* verts = (QuadVertex*)tvb.data;
	uint16_t* indices = (uint16_t*)tib.data;
	float texWidth    = (float)runTexture.width;
	float texHeight   = (float)runTexture.height;

	for (size_t i = runStart; i < runEnd; ++i)
	{
		const DrawItem& item = m_drawItems[i];
		glm::vec2 corners[4];
		ComputeQuadCorners(item.destRec, item.origin, item.rotation, corners);

		float u0      = item.sourceRec.x / texWidth;
		float v0      = item.sourceRec.y / texHeight;
		float u1      = (item.sourceRec.x + item.sourceRec.width) / texWidth;
		float v1      = (item.sourceRec.y + item.sourceRec.height) / texHeight;
		uint32_t abgr = PackColor(item.tint);

		uint32_t base   = (uint32_t)((i - runStart) * 4);
		verts[base + 0] = {corners[0].x, corners[0].y, u0, v0, abgr};
		verts[base + 1] = {corners[1].x, corners[1].y, u1, v0, abgr};
		verts[base + 2] = {corners[2].x, corners[2].y, u0, v1, abgr};
		verts[base + 3] = {corners[3].x, corners[3].y, u1, v1, abgr};

		uint16_t ibase                  = (uint16_t)base;
		indices[(i - runStart) * 6 + 0] = ibase + 0;
		indices[(i - runStart) * 6 + 1] = ibase + 1;
		indices[(i - runStart) * 6 + 2] = ibase + 2;
		indices[(i - runStart) * 6 + 3] = ibase + 2;
		indices[(i - runStart) * 6 + 4] = ibase + 1;
		indices[(i - runStart) * 6 + 5] = ibase + 3;
	}

	bgfx::setVertexBuffer(0, &tvb);
	bgfx::setIndexBuffer(&tib);
	bgfx::setTexture(0, texColorSampler, {(uint16_t)runTexture.id});
	bgfx::setState(drawState);
	bgfx::submit(GraphicsDevice::WorldViewId, runProgram);
}
