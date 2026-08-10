#include "WorldRenderer.h"

#include <algorithm>
#include <cfloat>
#include <cstring>

#include "Engine/Core/GameData.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/EmbeddedShaders.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/SpriteVertex.h"

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>

namespace
{
// Maps a float to a uint32 that sorts (as an unsigned integer) in the same order as the float,
// including negatives - lets orderInLayer (which can be a negative world Y) participate in the
// packed integer sort key correctly.
uint32_t FloatToSortableBits(float value)
{
	uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	uint32_t mask = (bits & 0x80000000u) ? 0xFFFFFFFFu : 0x80000000u;
	return bits ^ mask;
}

bool RectOverlaps(const Struktur::Util::Math::Rect& rect, const Struktur::Renderer::CullBounds& bounds)
{
	return rect.x < bounds.maxX && rect.x + rect.width > bounds.minX && rect.y < bounds.maxY &&
	      rect.y + rect.height > bounds.minY;
}
}  // namespace

Struktur::Renderer::WorldRenderer::WorldRenderer()
{
	m_drawItems.reserve(2048);
}

Struktur::Renderer::CullBounds Struktur::Renderer::WorldRenderer::ComputeCullBounds(GameContext& context)
{
	GameResource::Camera& camera = context.GetCamera();
	Core::GameData& gameData     = context.GetGameData();

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

uint64_t Struktur::Renderer::WorldRenderer::PackSortKey(GameResource::RenderLayer layer, float orderInLayer,
                                                        unsigned int textureId)
{
	// [63:60] layer (4 bits) | [59:28] orderInLayer, sortable-float-bits (32 bits) | [27:0] textureId (28 bits)
	uint64_t layerBits   = static_cast<uint64_t>(layer) & 0xFull;
	uint64_t orderBits   = static_cast<uint64_t>(FloatToSortableBits(orderInLayer));
	uint64_t textureBits = static_cast<uint64_t>(textureId) & 0xFFFFFFFull;
	return (layerBits << 60) | (orderBits << 28) | textureBits;
}

void Struktur::Renderer::WorldRenderer::Clear()
{
	m_drawItems.clear();
}

void Struktur::Renderer::WorldRenderer::Submit(GameResource::RenderLayer layer, float orderInLayer,
                                               entt::entity entity, const TextureHandle& texture,
                                               const Util::Math::Rect& sourceRec, const Util::Math::Rect& destRec,
                                               const glm::vec2& origin, float rotation,
                                               const Util::Math::Color& tint, const CullBounds& cullBounds)
{
	if (!RectOverlaps(destRec, cullBounds))
	{
		return;
	}

	DrawItem item{PackSortKey(layer, orderInLayer, texture.id), entity, texture, sourceRec, destRec, origin, rotation,
	             tint};
	m_drawItems.push_back(item);
}

void Struktur::Renderer::WorldRenderer::SubmitChunk(GameResource::RenderLayer layer, float orderInLayer,
                                                    const TileChunk& chunk, const TextureHandle& texture,
                                                    const CullBounds& cullBounds)
{
	if (!RectOverlaps(chunk.worldBounds, cullBounds))
	{
		return;
	}

	DrawItem item{PackSortKey(layer, orderInLayer, texture.id), entt::null, texture, {}, {}, {}, 0.0f, {}, &chunk};
	m_drawItems.push_back(item);
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
}  // namespace

void Struktur::Renderer::WorldRenderer::Flush(GameContext& context)
{
	std::sort(m_drawItems.begin(), m_drawItems.end(),
	          [](const DrawItem& a, const DrawItem& b) { return a.sortKey < b.sortKey; });

	Renderer::GraphicsDevice& graphicsDevice = context.GetGraphicsDevice();
	System::ShaderSystem& shaderSystem       = context.GetSystemManager().GetSystem<System::ShaderSystem>();
	GameResource::Camera& camera             = context.GetCamera();
	Core::GameData& gameData                 = context.GetGameData();

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = camera.GetProjectionMatrix(gameData.gameWidth, gameData.gameHeight);
	bgfx::setViewTransform(GraphicsDevice::WorldViewId, glm::value_ptr(view), glm::value_ptr(proj));

	static const bgfx::VertexLayout spriteLayout = BuildSpriteVertexLayout();
	static const bgfx::UniformHandle texColorSampler =
	    bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

	bgfx::ProgramHandle defaultProgram = graphicsDevice.GetDefaultSpriteProgram();
	uint64_t drawState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	                     BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	// The sort key already puts same-texture items adjacent; this walks the list once and additionally
	// resolves each sprite item's shader program, closing the current batch whenever either the texture or
	// the program changes and emitting exactly one bgfx draw call per contiguous run instead of one per
	// sprite. A chunk item always closes the run around it and submits its own cached buffer directly - it's
	// already a pre-built batch, so there's nothing to accumulate.
	size_t runStart                = 0;
	bgfx::ProgramHandle runProgram = BGFX_INVALID_HANDLE;
	Renderer::TextureHandle runTexture{0, 0, 0};

	auto flushRun = [&](size_t runEnd)
	{
		size_t count = runEnd - runStart;
		if (count == 0)
		{
			return;
		}

		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;
		bgfx::allocTransientVertexBuffer(&tvb, (uint32_t)(count * 4), spriteLayout);
		bgfx::allocTransientIndexBuffer(&tib, (uint32_t)(count * 6));

		SpriteVertex* verts  = (SpriteVertex*)tvb.data;
		uint16_t* indices    = (uint16_t*)tib.data;
		float texWidth       = (float)runTexture.width;
		float texHeight      = (float)runTexture.height;

		for (size_t i = runStart; i < runEnd; ++i)
		{
			const DrawItem& item = m_drawItems[i];
			glm::vec2 corners[4];
			ComputeQuadCorners(item.destRec, item.origin, item.rotation, corners);

			float u0 = item.sourceRec.x / texWidth;
			float v0 = item.sourceRec.y / texHeight;
			float u1 = (item.sourceRec.x + item.sourceRec.width) / texWidth;
			float v1 = (item.sourceRec.y + item.sourceRec.height) / texHeight;
			uint32_t abgr = PackColor(item.tint);

			uint32_t base                = (uint32_t)((i - runStart) * 4);
			verts[base + 0]              = {corners[0].x, corners[0].y, u0, v0, abgr};
			verts[base + 1]              = {corners[1].x, corners[1].y, u1, v0, abgr};
			verts[base + 2]              = {corners[2].x, corners[2].y, u0, v1, abgr};
			verts[base + 3]              = {corners[3].x, corners[3].y, u1, v1, abgr};

			uint16_t ibase      = (uint16_t)base;
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
	};

	for (size_t i = 0; i < m_drawItems.size(); ++i)
	{
		const DrawItem& item = m_drawItems[i];

		if (item.chunk)
		{
			flushRun(i);
			bgfx::setVertexBuffer(0, item.chunk->vb);
			bgfx::setIndexBuffer(item.chunk->ib);
			bgfx::setTexture(0, texColorSampler, {(uint16_t)item.texture.id});
			bgfx::setState(drawState);
			bgfx::submit(GraphicsDevice::WorldViewId, defaultProgram);
			runStart = i + 1;
			continue;
		}

		bgfx::ProgramHandle program = shaderSystem.ResolveProgram(context, item.entity, defaultProgram);

		bool boundary = (i == runStart)
		                    ? false
		                    : (item.texture.id != runTexture.id || program.idx != runProgram.idx);
		if (boundary)
		{
			flushRun(i);
			runStart = i;
		}
		runTexture = item.texture;
		runProgram = program;
	}
	flushRun(m_drawItems.size());
}
