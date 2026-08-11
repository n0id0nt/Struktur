#include "DebugRenderer.h"

#include "Debug/Assertions.h"
#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/EmbeddedShaders.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/SpriteVertex.h"

#include <glm/gtc/type_ptr.hpp>

Struktur::Renderer::DebugRenderer::DebugRenderer()
{
	m_texColorSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

	uint8_t whitePixel[4] = {255, 255, 255, 255};
	m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(whitePixel, 4));
	ASSERT_MSG(bgfx::isValid(m_whiteTexture), "Failed to create debug-renderer white texture");
}

Struktur::Renderer::DebugRenderer::~DebugRenderer()
{
	if (bgfx::isValid(m_whiteTexture))
	{
		bgfx::destroy(m_whiteTexture);
	}
}

void Struktur::Renderer::DebugRenderer::SetupView(GameContext& context)
{
	World::Camera& camera = context.GetCamera();
	Core::GameData& gameData     = context.GetGameData();
	glm::mat4 view                = camera.GetViewMatrix();
	glm::mat4 proj                = camera.GetProjectionMatrix(gameData.gameWidth, gameData.gameHeight);
	bgfx::setViewTransform(GraphicsDevice::DebugViewId, glm::value_ptr(view), glm::value_ptr(proj));
}

void Struktur::Renderer::DebugRenderer::SubmitTriangleFan(const glm::vec2* points, int count, uint32_t abgr)
{
	if (count < 3)
	{
		return;
	}

	static const bgfx::VertexLayout layout = BuildSpriteVertexLayout();
	uint32_t vertexCount                    = (uint32_t)count;
	uint32_t indexCount                     = (uint32_t)((count - 2) * 3);
	if (vertexCount > bgfx::getAvailTransientVertexBuffer(vertexCount, layout) ||
	    indexCount > bgfx::getAvailTransientIndexBuffer(indexCount))
	{
		return;
	}

	bgfx::TransientVertexBuffer tvb;
	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientVertexBuffer(&tvb, vertexCount, layout);
	bgfx::allocTransientIndexBuffer(&tib, indexCount);

	SpriteVertex* vertices = (SpriteVertex*)tvb.data;
	for (int i = 0; i < count; ++i)
	{
		vertices[i] = {points[i].x, points[i].y, 0.0f, 0.0f, abgr};
	}

	uint16_t* indices = (uint16_t*)tib.data;
	for (int i = 0; i < count - 2; ++i)
	{
		indices[i * 3 + 0] = 0;
		indices[i * 3 + 1] = (uint16_t)(i + 1);
		indices[i * 3 + 2] = (uint16_t)(i + 2);
	}

	bgfx::setTexture(0, m_texColorSampler, m_whiteTexture);
	bgfx::setVertexBuffer(0, &tvb, 0, vertexCount);
	bgfx::setIndexBuffer(&tib, 0, indexCount);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
	bgfx::submit(GraphicsDevice::DebugViewId, GetEmbeddedProgram("sprite"));
}

void Struktur::Renderer::DebugRenderer::DrawLine(const glm::vec2& p1, const glm::vec2& p2, float thickness,
                                                 const Util::Color& color)
{
	glm::vec2 dir = p2 - p1;
	float len     = glm::length(dir);
	if (len < 1e-5f)
	{
		return;
	}

	// Thick line as a rotated quad - generalises UIRenderer::DrawRectOutline's axis-aligned-only trick to
	// arbitrary angles, still needing no new shader or vertex layout.
	glm::vec2 normal = glm::vec2(-dir.y, dir.x) * (thickness * 0.5f / len);
	glm::vec2 quad[4] = {p1 - normal, p2 - normal, p2 + normal, p1 + normal};
	SubmitTriangleFan(quad, 4, PackColor(color));
}

void Struktur::Renderer::DebugRenderer::DrawPolygonOutline(const glm::vec2* points, int count, float thickness,
                                                           const Util::Color& color)
{
	for (int i = 0; i < count; ++i)
	{
		DrawLine(points[i], points[(i + 1) % count], thickness, color);
	}
}

void Struktur::Renderer::DebugRenderer::DrawSolidPolygon(const glm::vec2* points, int count, const Util::Color& color)
{
	SubmitTriangleFan(points, count, PackColor(color));
}

void Struktur::Renderer::DebugRenderer::DrawCircleOutline(const glm::vec2& center, float radius, float thickness,
                                                          const Util::Color& color)
{
	constexpr int kSegments = 24;
	glm::vec2 points[kSegments];
	for (int i = 0; i < kSegments; ++i)
	{
		float angle = (float)i / (float)kSegments * 2.0f * 3.14159265f;
		points[i]   = center + glm::vec2(cosf(angle), sinf(angle)) * radius;
	}
	DrawPolygonOutline(points, kSegments, thickness, color);
}

void Struktur::Renderer::DebugRenderer::DrawSolidCircle(const glm::vec2& center, float radius, const Util::Color& color)
{
	constexpr int kSegments = 24;
	glm::vec2 points[kSegments];
	for (int i = 0; i < kSegments; ++i)
	{
		float angle = (float)i / (float)kSegments * 2.0f * 3.14159265f;
		points[i]   = center + glm::vec2(cosf(angle), sinf(angle)) * radius;
	}
	SubmitTriangleFan(points, kSegments, PackColor(color));
}

void Struktur::Renderer::DebugRenderer::DrawRectOutline(const glm::vec2& min, const glm::vec2& max, float thickness,
                                                        const Util::Color& color)
{
	glm::vec2 points[4] = {min, {max.x, min.y}, max, {min.x, max.y}};
	DrawPolygonOutline(points, 4, thickness, color);
}
