#pragma once

#include "box2d/b2_draw.h"

namespace Struktur
{
namespace Renderer
{
class DebugRenderer;
}
}  // namespace Struktur

namespace Struktur
{
namespace Debug
{
// b2Draw implementation that forwards box2d's own internal body/fixture/joint/AABB walk (see
// b2World::SetDebugDraw/DebugDraw) to Renderer::DebugRenderer, converting box2d meters to pixels via the
// physics world's existing pixelsPerMeter convention. Desktop-only replacement for Box2DDebugRenderer's
// raylib-coupled manual walk - box2d's b2Draw callbacks are already resolved to world-space vertices/centers,
// so no shape-type switching or transform math is needed here (unlike Box2DDebugRenderer).
class Box2DBgfxDebugDraw : public b2Draw
{
public:
	explicit Box2DBgfxDebugDraw(Renderer::DebugRenderer& renderer);

	void SetPixelsPerMeter(float pixelsPerMeter)
	{
		m_pixelsPerMeter = pixelsPerMeter;
	}

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
	void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;
	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override;
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;
	void DrawTransform(const b2Transform& xf) override;
	void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

private:
	Renderer::DebugRenderer& m_renderer;
	float m_pixelsPerMeter = 32.0f;
};
}  // namespace Debug
}  // namespace Struktur
