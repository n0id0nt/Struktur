#pragma once

#include <cstdint>
#include <vector>

#include "Engine/UI/UIElement.h"
#include "Engine/Util/Color.h"

namespace Struktur
{
namespace UI
{
//=============================================================================
// UIGraph - line / area / step graphs drawn from axis-aligned quads (the UI renderer's only primitive), for the ride
// screens' power and heart-rate history and the workout profile.
//
// A series is a list of points with x in 0..1 across the element and y in the series' own units (mapped through its
// minimum..maximum to the element's height, so two series can use different scales). Segment i (point i -> i+1) is
// drawn in point i's colour, which is how the line follows the training zone sample by sample:
//   - line: a horizontal quad at y_i from x_i to x_{i+1}, plus a vertical quad joining y_i to y_{i+1}. With one point
//     per pixel or two this reads as a line; it is exact for step data (workout targets).
//   - fill: a column from the baseline (minimum) up to y_i, in point i's colour at `fillAlpha`.
// Markers are full-height vertical lines (the workout position marker).
//=============================================================================
class UIGraph : public UIElement
{
public:
	struct Point
	{
		float x           = 0.0f;
		float y           = 0.0f;
		Util::Color color = Util::ColorWhite;
	};

	struct Series
	{
		std::vector<Point> points;
		float minimum     = 0.0f;
		float maximum     = 1.0f;
		float thickness   = 2.0f;  // 0 = no line
		bool fill         = false;
		uint8_t fillAlpha = 90;
		int32_t zOffset   = 0;  // added to the element's z, so a target line can sit behind the actuals
	};

	struct Marker
	{
		float x           = 0.0f;
		float width       = 2.0f;
		Util::Color color = Util::ColorWhite;
	};

	UIGraph(const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const glm::vec2& absoluteSize,
	        const glm::vec2& relativeSize);

	void SetSeries(std::vector<Series> series);
	void SetMarkers(std::vector<Marker> markers);
	void Clear();

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;
	uint32_t GetRequiredQuadCount() const override;

private:
	uint32_t CountQuads() const;

	std::vector<Series> m_series;
	std::vector<Marker> m_markers;
};
}  // namespace UI
}  // namespace Struktur
