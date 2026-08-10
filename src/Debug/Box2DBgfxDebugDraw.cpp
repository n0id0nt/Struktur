#include "Box2DBgfxDebugDraw.h"

#include "Engine/Renderer/DebugRenderer.h"
#include "Engine/Util/MathUtil.h"

namespace
{
Struktur::Util::Math::Color ToColor(const b2Color& c)
{
	return Struktur::Util::Math::Color{(unsigned char)(c.r * 255.0f), (unsigned char)(c.g * 255.0f),
	                                    (unsigned char)(c.b * 255.0f), (unsigned char)(c.a * 255.0f)};
}
}  // namespace

Struktur::Debug::Box2DBgfxDebugDraw::Box2DBgfxDebugDraw(Renderer::DebugRenderer& renderer)
    : m_renderer(renderer)
{
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount,
                                                       const b2Color& color)
{
	glm::vec2 points[b2_maxPolygonVertices];
	for (int32 i = 0; i < vertexCount; ++i)
	{
		points[i] = glm::vec2(vertices[i].x, vertices[i].y) * m_pixelsPerMeter;
	}
	m_renderer.DrawPolygonOutline(points, vertexCount, 1.0f, ToColor(color));
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount,
                                                            const b2Color& color)
{
	glm::vec2 points[b2_maxPolygonVertices];
	for (int32 i = 0; i < vertexCount; ++i)
	{
		points[i] = glm::vec2(vertices[i].x, vertices[i].y) * m_pixelsPerMeter;
	}
	m_renderer.DrawSolidPolygon(points, vertexCount, ToColor(color));
	m_renderer.DrawPolygonOutline(points, vertexCount, 1.0f, ToColor(color));
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawCircle(const b2Vec2& center, float radius, const b2Color& color)
{
	m_renderer.DrawCircleOutline(glm::vec2(center.x, center.y) * m_pixelsPerMeter, radius * m_pixelsPerMeter, 1.0f,
	                             ToColor(color));
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis,
                                                           const b2Color& color)
{
	glm::vec2 pixelCenter = glm::vec2(center.x, center.y) * m_pixelsPerMeter;
	float pixelRadius     = radius * m_pixelsPerMeter;
	m_renderer.DrawSolidCircle(pixelCenter, pixelRadius, ToColor(color));
	// Radius line to show orientation, matching Box2DDebugRenderer's equivalent circle-shape drawing.
	glm::vec2 radiusEnd = pixelCenter + glm::vec2(axis.x, axis.y) * pixelRadius;
	m_renderer.DrawLine(pixelCenter, radiusEnd, 1.0f, ToColor(color));
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	m_renderer.DrawLine(glm::vec2(p1.x, p1.y) * m_pixelsPerMeter, glm::vec2(p2.x, p2.y) * m_pixelsPerMeter, 1.0f,
	                    ToColor(color));
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawTransform(const b2Transform& xf)
{
	// box2d-testbed convention: a short red/green axis cross at the transform's origin (used for e_centerOfMassBit,
	// i.e. one marker per body) - gives showPhysicsBodies a real per-body marker.
	constexpr float kAxisLength = 0.4f;
	glm::vec2 origin            = glm::vec2(xf.p.x, xf.p.y) * m_pixelsPerMeter;
	b2Vec2 xAxis                = xf.q.GetXAxis();
	b2Vec2 yAxis                 = xf.q.GetYAxis();

	m_renderer.DrawLine(origin, origin + glm::vec2(xAxis.x, xAxis.y) * kAxisLength * m_pixelsPerMeter, 1.0f,
	                    Util::Math::ColorRed);
	m_renderer.DrawLine(origin, origin + glm::vec2(yAxis.x, yAxis.y) * kAxisLength * m_pixelsPerMeter, 1.0f,
	                    Util::Math::ColorGreen);
}

void Struktur::Debug::Box2DBgfxDebugDraw::DrawPoint(const b2Vec2& p, float size, const b2Color& color)
{
	// size is already a small screen-pixel constant from box2d's own call sites (4-8px) - not a physics-space
	// length, so unlike center/radius/axis above it must NOT be scaled by m_pixelsPerMeter.
	m_renderer.DrawSolidCircle(glm::vec2(p.x, p.y) * m_pixelsPerMeter, size, ToColor(color));
}
