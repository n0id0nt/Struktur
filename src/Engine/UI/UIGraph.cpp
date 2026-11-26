#include "UIGraph.h"

#include <algorithm>

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"

Struktur::UI::UIGraph::UIGraph(const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
                               const glm::vec2& absoluteSize, const glm::vec2& relativeSize)
    : UIElement(absolutePosition, relativePosition, absoluteSize, relativeSize)
{
	m_focusable = false;
}

void Struktur::UI::UIGraph::SetSeries(std::vector<Series> series)
{
	m_series      = std::move(series);
	m_visualDirty = true;
}

void Struktur::UI::UIGraph::SetMarkers(std::vector<Marker> markers)
{
	m_markers     = std::move(markers);
	m_visualDirty = true;
}

void Struktur::UI::UIGraph::Clear()
{
	m_series.clear();
	m_markers.clear();
	m_visualDirty = true;
}

void Struktur::UI::UIGraph::Update(GameContext& context)
{
	UpdateChildren(context);
}

uint32_t Struktur::UI::UIGraph::CountQuads() const
{
	uint32_t count = (uint32_t)m_markers.size();
	for (const Series& series : m_series)
	{
		if (series.points.size() < 2)
		{
			continue;
		}
		const uint32_t segments = (uint32_t)series.points.size() - 1;
		count += series.thickness > 0.0f ? segments * 2 : 0;
		count += series.fill ? segments : 0;
	}
	return std::max<uint32_t>(1, count);
}

uint32_t Struktur::UI::UIGraph::GetRequiredQuadCount() const
{
	return CountQuads();
}

void Struktur::UI::UIGraph::Render(GameContext& context)
{
	if (m_visualDirty)
	{
		Renderer::UIRenderer& renderer = context.GetUIRenderer();
		const uint32_t required        = CountQuads();
		if (m_batchSlot.quadCapacity < required)
		{
			m_batchSlot = renderer.AllocateOrResizeSlot(m_batch, m_batchSlot, required);
		}

		const float left   = m_bounds.x;
		const float top    = m_bounds.y;
		const float width  = m_bounds.width;
		const float height = m_bounds.height;
		const float bottom = top + height;
		uint32_t used      = 0;

		// Each DrawRect writes into a one-quad view of the element's slot (4 vertices per quad).
		Renderer::UIBatchSlot quad = m_batchSlot;
		quad.quadCapacity          = 1;

		for (const Series& series : m_series)
		{
			if (series.points.size() < 2)
			{
				continue;
			}
			const float range    = series.maximum - series.minimum;
			const int32_t zIndex = GetZIndex() + series.zOffset;
			for (size_t i = 0; i + 1 < series.points.size(); ++i)
			{
				const Point& point = series.points[i];
				const Point& next  = series.points[i + 1];
				const float x0     = left + std::clamp(point.x, 0.0f, 1.0f) * width;
				const float x1     = left + std::clamp(next.x, 0.0f, 1.0f) * width;
				const float y0 =
				    bottom - std::clamp(range > 0.0f ? (point.y - series.minimum) / range : 0.0f, 0.0f, 1.0f) * height;
				const float y1 =
				    bottom - std::clamp(range > 0.0f ? (next.y - series.minimum) / range : 0.0f, 0.0f, 1.0f) * height;
				const float segmentWidth = std::max(0.0f, x1 - x0);
				if (series.fill)
				{
					Util::Color fillColor = point.color;
					fillColor.a           = series.fillAlpha;
					quad.vertexOffset     = m_batchSlot.vertexOffset + 4 * used++;
					renderer.DrawRect(m_batch, quad, Util::Math::Rect{x0, y0, segmentWidth, bottom - y0}, fillColor,
					                  zIndex);
				}
				if (series.thickness > 0.0f)
				{
					const float half  = series.thickness * 0.5f;
					quad.vertexOffset = m_batchSlot.vertexOffset + 4 * used++;
					renderer.DrawRect(m_batch, quad,
					                  Util::Math::Rect{x0, y0 - half, segmentWidth + half, series.thickness},
					                  point.color, zIndex + 1);
					const float joinTop = std::min(y0, y1) - half;
					quad.vertexOffset   = m_batchSlot.vertexOffset + 4 * used++;
					renderer.DrawRect(m_batch, quad,
					                  Util::Math::Rect{x1 - half, joinTop, series.thickness,
					                                   std::abs(y1 - y0) + series.thickness},
					                  point.color, zIndex + 1);
				}
			}
		}

		for (const Marker& marker : m_markers)
		{
			const float x     = left + std::clamp(marker.x, 0.0f, 1.0f) * width;
			quad.vertexOffset = m_batchSlot.vertexOffset + 4 * used++;
			renderer.DrawRect(m_batch, quad, Util::Math::Rect{x - marker.width * 0.5f, top, marker.width, height},
			                  marker.color, GetZIndex() + 3);
		}

		renderer.ClearSlotFrom(m_batch, m_batchSlot, used);
		m_visualDirty = false;
	}

	RenderChildren(context);
}
