#include "Placeholder.h"

#include <algorithm>
#include <cmath>

#include "Engine/World/Noise.h"

namespace Struktur::World
{
static constexpr double PI = 3.14159265358979323846;
static constexpr int MAX_PLACEHOLDER_DIMENSION = 2048;

struct Rgba
{
	uint8_t red   = 255;
	uint8_t green = 255;
	uint8_t blue  = 255;
	uint8_t alpha = 255;
};

static Rgba FromPacked(uint32_t packed)
{
	return Rgba{static_cast<uint8_t>(packed >> 24), static_cast<uint8_t>(packed >> 16), static_cast<uint8_t>(packed >> 8),
	            static_cast<uint8_t>(packed)};
}

static Rgba Shade(const Rgba& color, double factor)
{
	return Rgba{static_cast<uint8_t>(std::clamp(color.red * factor, 0.0, 255.0)),
	            static_cast<uint8_t>(std::clamp(color.green * factor, 0.0, 255.0)),
	            static_cast<uint8_t>(std::clamp(color.blue * factor, 0.0, 255.0)), color.alpha};
}

// Draws into one frame of the image (origin at the frame's top-left).
class Canvas
{
public:
	Canvas(PlaceholderImage& image, int frameWidth, int frameHeight)
	    : m_image(image),
	      m_frameWidth(frameWidth),
	      m_frameHeight(frameHeight),
	      m_originX(0),
	      m_originY(0)
	{
	}

	void SelectFrame(int column, int row)
	{
		m_originX = column * m_frameWidth;
		m_originY = row * m_frameHeight;
	}

	int Width() const
	{
		return m_frameWidth;
	}
	int Height() const
	{
		return m_frameHeight;
	}

	// Alpha-blends `color` (with `coverage` 0-1) over the pixel.
	void Blend(int x, int y, const Rgba& color, double coverage = 1.0)
	{
		if (x < 0 || y < 0 || x >= m_frameWidth || y >= m_frameHeight)
		{
			return;
		}
		const size_t offset =
		    (static_cast<size_t>(m_originY + y) * static_cast<size_t>(m_image.width) + static_cast<size_t>(m_originX + x)) * 4;
		const double sourceAlpha = std::clamp(color.alpha / 255.0 * coverage, 0.0, 1.0);
		const double destAlpha   = m_image.pixels[offset + 3] / 255.0;
		const double outAlpha    = sourceAlpha + destAlpha * (1.0 - sourceAlpha);
		if (outAlpha <= 0.0)
		{
			return;
		}
		const uint8_t source[3] = {color.red, color.green, color.blue};
		for (int channel = 0; channel < 3; ++channel)
		{
			const double mixed = (source[channel] * sourceAlpha + m_image.pixels[offset + channel] * destAlpha * (1.0 - sourceAlpha)) / outAlpha;
			m_image.pixels[offset + channel] = static_cast<uint8_t>(std::clamp(mixed, 0.0, 255.0));
		}
		m_image.pixels[offset + 3] = static_cast<uint8_t>(std::clamp(outAlpha * 255.0, 0.0, 255.0));
	}

	void Set(int x, int y, const Rgba& color)
	{
		if (x < 0 || y < 0 || x >= m_frameWidth || y >= m_frameHeight)
		{
			return;
		}
		const size_t offset =
		    (static_cast<size_t>(m_originY + y) * static_cast<size_t>(m_image.width) + static_cast<size_t>(m_originX + x)) * 4;
		m_image.pixels[offset]     = color.red;
		m_image.pixels[offset + 1] = color.green;
		m_image.pixels[offset + 2] = color.blue;
		m_image.pixels[offset + 3] = color.alpha;
	}

	void FillRect(double left, double top, double right, double bottom, const Rgba& color)
	{
		for (int y = static_cast<int>(std::floor(top)); y < static_cast<int>(std::ceil(bottom)); ++y)
		{
			for (int x = static_cast<int>(std::floor(left)); x < static_cast<int>(std::ceil(right)); ++x)
			{
				Blend(x, y, color);
			}
		}
	}

	// Ellipse with a soft edge of `softness` pixels (0 = hard).
	void FillEllipse(double centerX, double centerY, double radiusX, double radiusY, const Rgba& color, double softness = 0.0)
	{
		const int left   = static_cast<int>(std::floor(centerX - radiusX - softness - 1));
		const int right  = static_cast<int>(std::ceil(centerX + radiusX + softness + 1));
		const int top    = static_cast<int>(std::floor(centerY - radiusY - softness - 1));
		const int bottom = static_cast<int>(std::ceil(centerY + radiusY + softness + 1));
		for (int y = top; y <= bottom; ++y)
		{
			for (int x = left; x <= right; ++x)
			{
				const double deltaX   = (x + 0.5 - centerX) / std::max(0.5, radiusX);
				const double deltaY   = (y + 0.5 - centerY) / std::max(0.5, radiusY);
				const double distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
				if (softness <= 0.0)
				{
					if (distance <= 1.0)
					{
						Blend(x, y, color);
					}
					continue;
				}
				const double edge     = 1.0 - softness / std::max(radiusX, radiusY);
				const double coverage = std::clamp((1.0 - distance) / std::max(0.01, 1.0 - edge), 0.0, 1.0);
				if (coverage > 0.0)
				{
					Blend(x, y, color, coverage);
				}
			}
		}
	}

	void FillTriangle(double x0, double y0, double x1, double y1, double x2, double y2, const Rgba& color)
	{
		const int left   = static_cast<int>(std::floor(std::min({x0, x1, x2})));
		const int right  = static_cast<int>(std::ceil(std::max({x0, x1, x2})));
		const int top    = static_cast<int>(std::floor(std::min({y0, y1, y2})));
		const int bottom = static_cast<int>(std::ceil(std::max({y0, y1, y2})));
		const double area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
		if (std::abs(area) < 1e-9)
		{
			return;
		}
		for (int y = top; y <= bottom; ++y)
		{
			for (int x = left; x <= right; ++x)
			{
				const double pointX = x + 0.5;
				const double pointY = y + 0.5;
				const double weight0 = ((x1 - pointX) * (y2 - pointY) - (x2 - pointX) * (y1 - pointY)) / area;
				const double weight1 = ((x2 - pointX) * (y0 - pointY) - (x0 - pointX) * (y2 - pointY)) / area;
				const double weight2 = 1.0 - weight0 - weight1;
				if (weight0 >= 0.0 && weight1 >= 0.0 && weight2 >= 0.0)
				{
					Blend(x, y, color);
				}
			}
		}
	}

	void Line(double x0, double y0, double x1, double y1, double thickness, const Rgba& color)
	{
		const double length = std::hypot(x1 - x0, y1 - y0);
		const int steps     = std::max(1, static_cast<int>(std::ceil(length * 2.0)));
		for (int step = 0; step <= steps; ++step)
		{
			const double fraction = static_cast<double>(step) / steps;
			FillEllipse(x0 + (x1 - x0) * fraction, y0 + (y1 - y0) * fraction, thickness * 0.5, thickness * 0.5, color);
		}
	}

private:
	PlaceholderImage& m_image;
	int m_frameWidth;
	int m_frameHeight;
	int m_originX;
	int m_originY;
};

static const Rgba OUTLINE{20, 20, 24, 255};

// FNV-1a: stable across compilers (std::hash isn't).
static uint64_t HashText(const std::string& text)
{
	uint64_t value = 1469598103934665603ull;
	for (char character : text)
	{
		value ^= static_cast<uint8_t>(character);
		value *= 1099511628211ull;
	}
	return value;
}

static void DrawPine(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	const double trunk  = width * 0.08;
	canvas.FillRect(width * 0.5 - trunk, height * 0.82, width * 0.5 + trunk, height, Rgba{92, 64, 40, 255});
	const int layers = 3 + static_cast<int>(random.NextUnit() * 2.0);
	for (int layer = 0; layer < layers; ++layer)
	{
		const double fraction = static_cast<double>(layer) / layers;
		const double top      = height * (0.02 + fraction * 0.55);
		const double bottom   = height * (0.40 + fraction * 0.45);
		const double halfBase = width * (0.22 + fraction * 0.24) * random.Range(0.9, 1.1);
		canvas.FillTriangle(width * 0.5, top, width * 0.5 - halfBase, bottom, width * 0.5 + halfBase, bottom,
		                    Shade(color, 0.85 + fraction * 0.3));
	}
}

static void DrawBroadleaf(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	canvas.FillRect(width * 0.45, height * 0.55, width * 0.55, height, Rgba{96, 70, 44, 255});
	for (int blob = 0; blob < 7; ++blob)
	{
		const double centerX = width * random.Range(0.28, 0.72);
		const double centerY = height * random.Range(0.18, 0.5);
		const double radius  = width * random.Range(0.18, 0.3);
		canvas.FillEllipse(centerX, centerY, radius, radius * 0.9, Shade(color, random.Range(0.8, 1.15)));
	}
}

static void DrawPalm(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	const double lean   = random.Range(-0.12, 0.12);
	double previousX    = width * 0.5;
	double previousY    = height;
	for (int segment = 1; segment <= 10; ++segment)
	{
		const double fraction = segment / 10.0;
		const double x        = width * (0.5 + lean * fraction * fraction);
		const double y        = height * (1.0 - fraction * 0.72);
		canvas.Line(previousX, previousY, x, y, width * 0.07, Rgba{130, 100, 64, 255});
		previousX = x;
		previousY = y;
	}
	for (int frond = 0; frond < 7; ++frond)
	{
		const double angle = PI * (0.08 + frond / 6.0 * 0.84) + random.Range(-0.1, 0.1);
		const double reach = width * random.Range(0.32, 0.45);
		canvas.Line(previousX, previousY, previousX - std::cos(angle) * reach, previousY + std::sin(angle) * reach * 0.35 - reach * 0.15,
		            width * 0.06, Shade(color, random.Range(0.85, 1.1)));
	}
}

static void DrawCactus(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	canvas.FillRect(width * 0.42, height * 0.08, width * 0.58, height, color);
	canvas.FillEllipse(width * 0.5, height * 0.08, width * 0.08, width * 0.08, color);
	const double armLeft  = height * random.Range(0.35, 0.55);
	const double armRight = height * random.Range(0.3, 0.5);
	canvas.FillRect(width * 0.2, armLeft, width * 0.42, armLeft + width * 0.12, color);
	canvas.FillRect(width * 0.2, armLeft - height * 0.2, width * 0.3, armLeft + width * 0.12, color);
	canvas.FillRect(width * 0.58, armRight, width * 0.8, armRight + width * 0.12, color);
	canvas.FillRect(width * 0.7, armRight - height * 0.18, width * 0.8, armRight + width * 0.12, color);
}

static void DrawBush(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	for (int blob = 0; blob < 6; ++blob)
	{
		const double radius = width * random.Range(0.16, 0.26);
		canvas.FillEllipse(width * random.Range(0.25, 0.75), height - radius * random.Range(0.8, 1.4), radius, radius * 0.85,
		                   Shade(color, random.Range(0.8, 1.15)));
	}
}

static void DrawRock(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	double previousX    = width * 0.08;
	double previousY    = height;
	const int points    = 6;
	for (int point = 1; point <= points; ++point)
	{
		const double angle = PI * point / points;
		const double x     = width * (0.5 - 0.42 * std::cos(angle));
		const double y     = height * (1.0 - std::sin(angle) * random.Range(0.6, 0.95));
		canvas.FillTriangle(width * 0.5, height, previousX, previousY, x, y, Shade(color, random.Range(0.8, 1.15)));
		previousX = x;
		previousY = y;
	}
}

static void DrawGrass(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	for (int blade = 0; blade < 14; ++blade)
	{
		const double baseX = width * random.Range(0.1, 0.9);
		canvas.Line(baseX, height, baseX + width * random.Range(-0.15, 0.15), height * random.Range(0.05, 0.5), 1.4,
		            Shade(color, random.Range(0.75, 1.2)));
	}
}

static void DrawSign(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	canvas.FillRect(width * 0.46, height * 0.3, width * 0.54, height, Rgba{150, 150, 150, 255});
	canvas.FillRect(width * 0.1, height * 0.04, width * 0.9, height * 0.42, OUTLINE);
	canvas.FillRect(width * 0.14, height * 0.07, width * 0.86, height * 0.39, color);
	const double arrow = random.NextUnit() < 0.5 ? -1.0 : 1.0;
	canvas.FillTriangle(width * (0.5 + 0.25 * arrow), height * 0.23, width * 0.5, height * 0.12, width * 0.5, height * 0.34,
	                    Rgba{250, 250, 250, 255});
}

static void DrawGuardrail(Canvas& canvas, const Rgba& color)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	canvas.FillRect(width * 0.05, height * 0.3, width * 0.13, height, Rgba{110, 110, 112, 255});
	canvas.FillRect(width * 0.87, height * 0.3, width * 0.95, height, Rgba{110, 110, 112, 255});
	canvas.FillRect(0, height * 0.2, width, height * 0.55, color);
	canvas.FillRect(0, height * 0.32, width, height * 0.42, Shade(color, 0.8));
}

static void DrawLamp(Canvas& canvas, const Rgba& color)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	canvas.FillRect(width * 0.45, height * 0.06, width * 0.55, height, Rgba{80, 84, 90, 255});
	canvas.FillRect(width * 0.2, height * 0.04, width * 0.55, height * 0.09, Rgba{80, 84, 90, 255});
	canvas.FillEllipse(width * 0.24, height * 0.1, width * 0.12, width * 0.07, color, 2.0);
}

static void DrawBuilding(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	const double top    = height * random.Range(0.05, 0.35);
	canvas.FillRect(width * 0.04, top, width * 0.96, height, color);
	canvas.FillTriangle(width * 0.0, top, width * 1.0, top, width * 0.5, top - height * 0.12, Shade(color, 0.6));
	const Rgba window = random.NextUnit() < 0.5 ? Rgba{255, 230, 140, 255} : Rgba{60, 80, 110, 255};
	for (double y = top + height * 0.06; y < height * 0.86; y += height * 0.14)
	{
		for (double x = width * 0.12; x < width * 0.86; x += width * 0.2)
		{
			canvas.FillRect(x, y, x + width * 0.1, y + height * 0.07, window);
		}
	}
}

static void DrawBanner(Canvas& canvas, const Rgba& color)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	canvas.FillRect(width * 0.02, height * 0.0, width * 0.05, height, Rgba{200, 200, 200, 255});
	canvas.FillRect(width * 0.95, height * 0.0, width * 0.98, height, Rgba{200, 200, 200, 255});
	canvas.FillRect(width * 0.05, height * 0.02, width * 0.95, height * 0.3, color);
	for (int square = 0; square < 16; ++square)
	{
		const double left = width * (0.05 + square * 0.05625);
		const Rgba tone   = square % 2 == 0 ? Rgba{20, 20, 20, 255} : Rgba{240, 240, 240, 255};
		canvas.FillRect(left, height * 0.24, left + width * 0.05625, height * 0.3, tone);
	}
}

static void DrawCloud(Canvas& canvas, const Rgba& color, Random& random)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	for (int puff = 0; puff < 8; ++puff)
	{
		const double radius = height * random.Range(0.18, 0.34);
		canvas.FillEllipse(width * random.Range(0.2, 0.8), height * random.Range(0.45, 0.65), radius * 1.5, radius, color,
		                   radius * 0.5);
	}
}

static void DrawSun(Canvas& canvas, const Rgba& color)
{
	const double size = canvas.Width();
	canvas.FillEllipse(size * 0.5, size * 0.5, size * 0.48, size * 0.48, Rgba{color.red, color.green, color.blue, 90},
	                   size * 0.3);
	canvas.FillEllipse(size * 0.5, size * 0.5, size * 0.24, size * 0.24, color, 1.0);
}

static void DrawMoon(Canvas& canvas, const Rgba& color, Random& random)
{
	const double size = canvas.Width();
	canvas.FillEllipse(size * 0.5, size * 0.5, size * 0.46, size * 0.46, Rgba{color.red, color.green, color.blue, 60},
	                   size * 0.2);
	canvas.FillEllipse(size * 0.5, size * 0.5, size * 0.28, size * 0.28, color, 1.0);
	for (int crater = 0; crater < 5; ++crater)
	{
		canvas.FillEllipse(size * random.Range(0.36, 0.64), size * random.Range(0.36, 0.64), size * 0.04, size * 0.04,
		                   Shade(color, 0.8));
	}
}

static void DrawStars(Canvas& canvas, Random& random)
{
	const int count = canvas.Width() * canvas.Height() / 180;
	for (int star = 0; star < count; ++star)
	{
		const int x            = static_cast<int>(random.NextUnit() * canvas.Width());
		const int y            = static_cast<int>(random.NextUnit() * canvas.Height());
		const uint8_t brightness = static_cast<uint8_t>(random.Range(120.0, 255.0));
		canvas.Set(x, y, Rgba{brightness, brightness, static_cast<uint8_t>(std::min(255, brightness + 20)), brightness});
	}
}

static void DrawMountains(Canvas& canvas, uint64_t seed, int column)
{
	// White silhouette (tinted in the shader), tileable horizontally.
	const int width  = canvas.Width();
	const int height = canvas.Height();
	for (int x = 0; x < width; ++x)
	{
		const double position = static_cast<double>(x) / width;
		// Periodic: sum of sines with integer frequencies wraps exactly.
		double ridge = 0.0;
		for (int harmonic = 1; harmonic <= 6; ++harmonic)
		{
			const double phase = HashUnit(seed, column, harmonic) * 2.0 * PI;
			ridge += std::sin(position * 2.0 * PI * harmonic * 2 + phase) / harmonic;
		}
		const int top = static_cast<int>(height * (0.45 - ridge * 0.22));
		for (int y = std::max(0, top); y < height; ++y)
		{
			canvas.Set(x, y, Rgba{255, 255, 255, 255});
		}
	}
}

static void DrawNoise(Canvas& canvas, const Rgba& color, uint64_t seed, double contrast)
{
	const int width  = canvas.Width();
	const int height = canvas.Height();
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// Tileable: hash on wrapped lattice cells.
			const double value = HashSigned(seed, x % width, y % height) * 0.6 +
			                     HashSigned(seed ^ 0x77u, (x / 4) % (width / 4 + 1), (y / 4) % (height / 4 + 1)) * 0.4;
			canvas.Set(x, y, Shade(color, 1.0 + value * contrast));
		}
	}
}

static void DrawWater(Canvas& canvas, const Rgba& color, uint64_t seed)
{
	DrawNoise(canvas, color, seed, 0.08);
	const int width  = canvas.Width();
	const int height = canvas.Height();
	Random random(seed);
	for (int ripple = 0; ripple < width / 3; ++ripple)
	{
		const int x      = static_cast<int>(random.NextUnit() * width);
		const int y      = static_cast<int>(random.NextUnit() * height);
		const int length = 2 + static_cast<int>(random.NextUnit() * 6);
		for (int offset = 0; offset < length; ++offset)
		{
			canvas.Set((x + offset) % width, y, Shade(color, 1.35));
		}
	}
}

static void DrawRain(Canvas& canvas)
{
	canvas.Line(canvas.Width() * 0.5, 0, canvas.Width() * 0.5, canvas.Height(), 1.0, Rgba{200, 215, 235, 190});
}

static void DrawSnow(Canvas& canvas)
{
	canvas.FillEllipse(canvas.Width() * 0.5, canvas.Height() * 0.5, canvas.Width() * 0.35, canvas.Height() * 0.35,
	                   Rgba{250, 250, 255, 255}, 1.5);
}

static void DrawShadow(Canvas& canvas)
{
	canvas.FillEllipse(canvas.Width() * 0.5, canvas.Height() * 0.5, canvas.Width() * 0.48, canvas.Height() * 0.45,
	                   Rgba{0, 0, 0, 150}, canvas.Height() * 0.4);
}

// --- rider (seen from behind) ---------------------------------------------------------------------------------------

static void DrawRiderFrame(Canvas& canvas)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	const Rgba metal{70, 74, 82, 255};
	canvas.Line(width * 0.5, height * 0.56, width * 0.5, height * 0.8, 2.5, metal);       // seat tube
	canvas.FillEllipse(width * 0.5, height * 0.555, width * 0.09, height * 0.018, OUTLINE);  // saddle
	canvas.Line(width * 0.22, height * 0.45, width * 0.78, height * 0.45, 2.0, metal);     // handlebar (behind torso)
	canvas.FillRect(width * 0.53, height * 0.84, width * 0.6, height * 0.9, Rgba{40, 40, 44, 255});  // derailleur
}

static void DrawRiderWheel(Canvas& canvas)
{
	// A tileable strip of tyre tread, scrolled vertically by the renderer as the wheel turns.
	const int width  = canvas.Width();
	const int height = canvas.Height();
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const bool edge  = x == 0 || x == width - 1;
			const bool tread = (y / 3) % 2 == 0 && !edge;
			canvas.Set(x, y, edge ? Rgba{12, 12, 14, 255} : (tread ? Rgba{58, 58, 62, 255} : Rgba{30, 30, 34, 255}));
		}
	}
}

static void DrawLegsFrame(Canvas& canvas, double hipY, double crankAngle, bool idle)
{
	const double width   = canvas.Width();
	const double height  = canvas.Height();
	const double crankY  = height * 0.79;
	const double crankR  = height * 0.075;
	const Rgba shorts{24, 24, 28, 255};
	const Rgba skin{224, 180, 140, 255};
	const Rgba shoe{235, 235, 235, 255};
	for (int side = 0; side < 2; ++side)
	{
		const double sign   = side == 0 ? -1.0 : 1.0;
		const double hipX   = width * 0.5 + sign * width * 0.07;
		double footX        = width * 0.5 + sign * width * 0.1;
		// From behind the pedals move up and down; the two feet are half a revolution apart.
		double footY        = crankY - std::cos(crankAngle + side * PI) * crankR;
		if (idle && side == 0)
		{
			footX = width * 0.5 - width * 0.24;  // unclipped foot on the ground
			footY = height * 0.97;
		}
		const double legLength = height * 0.36;
		const double reach     = std::hypot(footX - hipX, footY - hipY);
		// Knee pushed outwards so the bend shows from behind.
		const double bend  = std::sqrt(std::max(0.0, legLength * legLength * 0.25 - reach * reach * 0.25));
		const double kneeX = (hipX + footX) * 0.5 + sign * bend * 0.6;
		const double kneeY = (hipY + footY) * 0.5 - bend * 0.25;
		canvas.Line(hipX, hipY, kneeX, kneeY, width * 0.11, shorts);
		canvas.Line(kneeX, kneeY, footX, footY - 3.0, width * 0.08, skin);
		canvas.FillEllipse(footX, footY, width * 0.06, height * 0.022, shoe);
	}
}

static void DrawRiderLegs(Canvas& canvas, int framesPerRevolution, int rows)
{
	const double height = canvas.Height();
	for (int row = 0; row < rows; ++row)
	{
		for (int frame = 0; frame < framesPerRevolution; ++frame)
		{
			canvas.SelectFrame(frame, row);
			const double angle = 2.0 * PI * frame / framesPerRevolution;
			if (row == 0)
			{
				DrawLegsFrame(canvas, height * 0.56, angle, false);
			}
			else if (row == 1)
			{
				DrawLegsFrame(canvas, height * 0.48, angle, false);  // out of the saddle: hips higher
			}
			else if (frame == 0)
			{
				DrawLegsFrame(canvas, height * 0.56, PI * 0.5, false);  // coasting: cranks level
			}
			else if (frame == 1)
			{
				DrawLegsFrame(canvas, height * 0.56, 0.0, true);  // idle: one foot down
			}
		}
	}
}

static void DrawRiderTorso(Canvas& canvas, const Rgba& jersey)
{
	const double width  = canvas.Width();
	const double height = canvas.Height();
	const Rgba skin{224, 180, 140, 255};
	// Arms to the bars, then the back (tapered), then the head.
	canvas.Line(width * 0.3, height * 0.27, width * 0.22, height * 0.45, width * 0.08, skin);
	canvas.Line(width * 0.7, height * 0.27, width * 0.78, height * 0.45, width * 0.08, skin);
	canvas.FillTriangle(width * 0.27, height * 0.24, width * 0.73, height * 0.24, width * 0.5, height * 0.6, jersey);
	canvas.FillRect(width * 0.36, height * 0.24, width * 0.64, height * 0.57, jersey);
	canvas.FillRect(width * 0.38, height * 0.5, width * 0.62, height * 0.58, Shade(jersey, 0.7));
	canvas.FillEllipse(width * 0.5, height * 0.17, width * 0.1, height * 0.065, Rgba{235, 235, 240, 255});  // helmet
	canvas.FillRect(width * 0.47, height * 0.21, width * 0.53, height * 0.25, skin);  // neck
}

PlaceholderImage ComposeRegions(const uint8_t* pixels, int width, int height, const AssetRole& role)
{
	PlaceholderImage image;
	if (role.regions.empty() || pixels == nullptr || width <= 0 || height <= 0)
	{
		return image;
	}
	int cellWidth  = 1;
	int cellHeight = 1;
	for (const std::array<int, 4>& region : role.regions)
	{
		cellWidth  = std::max(cellWidth, region[2]);
		cellHeight = std::max(cellHeight, region[3]);
	}
	const int cells = static_cast<int>(role.regions.size());
	image.width     = std::min(cellWidth * cells, MAX_PLACEHOLDER_DIMENSION * 4);
	image.height    = cellHeight;
	image.pixels.assign(static_cast<size_t>(image.width) * image.height * 4, 0);
	for (int cell = 0; cell < cells; ++cell)
	{
		const std::array<int, 4>& region = role.regions[static_cast<size_t>(cell)];
		const int offsetX = cell * cellWidth + (cellWidth - region[2]) / 2;
		const int offsetY = cellHeight - region[3];
		for (int row = 0; row < region[3]; ++row)
		{
			const int sourceY = region[1] + row;
			if (sourceY < 0 || sourceY >= height)
			{
				continue;
			}
			for (int column = 0; column < region[2]; ++column)
			{
				const int sourceX = region[0] + column;
				const int targetX = offsetX + column;
				if (sourceX < 0 || sourceX >= width || targetX >= image.width)
				{
					continue;
				}
				const size_t source = (static_cast<size_t>(sourceY) * width + sourceX) * 4;
				const size_t target = (static_cast<size_t>(offsetY + row) * image.width + targetX) * 4;
				for (int channel = 0; channel < 4; ++channel)
				{
					image.pixels[target + channel] = pixels[source + channel];
				}
			}
		}
	}
	return image;
}

const std::vector<std::string>& KnownPlaceholderShapes()
{
	static const std::vector<std::string> shapes = {
	    "tree_pine", "tree_broadleaf", "tree_palm", "cactus", "bush", "rock", "grass", "sign", "guardrail",
	    "lamp", "building", "banner", "cloud", "sun", "moon", "stars", "mountains", "noise", "water", "markings",
	    "rain", "snow", "shadow", "rider_frame", "rider_wheel", "rider_legs", "rider_torso", "box"};
	return shapes;
}

PlaceholderImage GeneratePlaceholder(const AssetRole& role, uint64_t seed)
{
	PlaceholderImage image;
	const int frameWidth  = std::clamp(role.width, 1, MAX_PLACEHOLDER_DIMENSION);
	const int frameHeight = std::clamp(role.height, 1, MAX_PLACEHOLDER_DIMENSION);
	const int columns     = std::clamp(role.frames, 1, MAX_PLACEHOLDER_DIMENSION / frameWidth);
	const int rows        = std::clamp(role.rows, 1, MAX_PLACEHOLDER_DIMENSION / frameHeight);
	image.width           = frameWidth * columns;
	image.height          = frameHeight * rows;
	image.pixels.assign(static_cast<size_t>(image.width) * image.height * 4, 0);
	Canvas canvas(image, frameWidth, frameHeight);
	const Rgba color      = FromPacked(role.placeholderColor);
	const std::string& shape = role.placeholderShape;
	const uint64_t roleSeed  = Hash(seed, static_cast<int64_t>(HashText(role.role) & 0x7FFFFFFFFFFFull));

	if (shape == "rider_legs")
	{
		DrawRiderLegs(canvas, columns, rows);
		return image;
	}
	for (int row = 0; row < rows; ++row)
	{
		for (int column = 0; column < columns; ++column)
		{
			canvas.SelectFrame(column, row);
			Random random(Hash(roleSeed, column, row));
			if (shape == "tree_pine")
			{
				DrawPine(canvas, color, random);
			}
			else if (shape == "tree_broadleaf")
			{
				DrawBroadleaf(canvas, color, random);
			}
			else if (shape == "tree_palm")
			{
				DrawPalm(canvas, color, random);
			}
			else if (shape == "cactus")
			{
				DrawCactus(canvas, color, random);
			}
			else if (shape == "bush")
			{
				DrawBush(canvas, color, random);
			}
			else if (shape == "rock")
			{
				DrawRock(canvas, color, random);
			}
			else if (shape == "grass")
			{
				DrawGrass(canvas, color, random);
			}
			else if (shape == "sign")
			{
				DrawSign(canvas, color, random);
			}
			else if (shape == "guardrail")
			{
				DrawGuardrail(canvas, color);
			}
			else if (shape == "lamp")
			{
				DrawLamp(canvas, color);
			}
			else if (shape == "building")
			{
				DrawBuilding(canvas, color, random);
			}
			else if (shape == "banner")
			{
				DrawBanner(canvas, color);
			}
			else if (shape == "cloud")
			{
				DrawCloud(canvas, color, random);
			}
			else if (shape == "sun")
			{
				DrawSun(canvas, color);
			}
			else if (shape == "moon")
			{
				DrawMoon(canvas, color, random);
			}
			else if (shape == "stars")
			{
				DrawStars(canvas, random);
			}
			else if (shape == "mountains")
			{
				DrawMountains(canvas, roleSeed, column);
			}
			else if (shape == "noise")
			{
				DrawNoise(canvas, color, Hash(roleSeed, column), 0.12);
			}
			else if (shape == "markings")
			{
				DrawNoise(canvas, color, Hash(roleSeed, column), 0.05);
			}
			else if (shape == "water")
			{
				DrawWater(canvas, color, Hash(roleSeed, column));
			}
			else if (shape == "rain")
			{
				DrawRain(canvas);
			}
			else if (shape == "snow")
			{
				DrawSnow(canvas);
			}
			else if (shape == "shadow")
			{
				DrawShadow(canvas);
			}
			else if (shape == "rider_frame")
			{
				DrawRiderFrame(canvas);
			}
			else if (shape == "rider_wheel")
			{
				DrawRiderWheel(canvas);
			}
			else if (shape == "rider_torso")
			{
				DrawRiderTorso(canvas, color);
			}
			else
			{
				// Unknown / "box": an obvious checker so a missing shape is easy to spot.
				for (int y = 0; y < frameHeight; ++y)
				{
					for (int x = 0; x < frameWidth; ++x)
					{
						const bool dark = ((x / 8) + (y / 8)) % 2 == 0;
						canvas.Set(x, y, dark ? Rgba{20, 20, 20, 255} : Rgba{255, 0, 255, 255});
					}
				}
			}
		}
	}
	return image;
}
}  // namespace Struktur::World
