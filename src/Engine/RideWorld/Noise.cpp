#include "Noise.h"

#include <cmath>

namespace Struktur::RideWorld
{
uint64_t Hash64(uint64_t value)
{
	value += 0x9E3779B97F4A7C15ull;
	value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
	value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
	return value ^ (value >> 31);
}

uint64_t Hash(uint64_t seed, int64_t x, int64_t y, int64_t z)
{
	uint64_t value = Hash64(seed);
	value          = Hash64(value ^ static_cast<uint64_t>(x));
	value          = Hash64(value ^ (static_cast<uint64_t>(y) * 0xD6E8FEB86659FD93ull));
	value          = Hash64(value ^ (static_cast<uint64_t>(z) * 0xA0761D6478BD642Full));
	return value;
}

double HashUnit(uint64_t seed, int64_t x, int64_t y, int64_t z)
{
	// Top 53 bits -> [0, 1).
	return static_cast<double>(Hash(seed, x, y, z) >> 11) * (1.0 / 9007199254740992.0);
}

double HashSigned(uint64_t seed, int64_t x, int64_t y, int64_t z)
{
	return HashUnit(seed, x, y, z) * 2.0 - 1.0;
}

static double SmoothStep(double fraction)
{
	return fraction * fraction * (3.0 - 2.0 * fraction);
}

double ValueNoise1D(uint64_t seed, double x)
{
	const double cell     = std::floor(x);
	const int64_t index   = static_cast<int64_t>(cell);
	const double weight   = SmoothStep(x - cell);
	const double left     = HashSigned(seed, index);
	const double right    = HashSigned(seed, index + 1);
	return left + (right - left) * weight;
}

double ValueNoise2D(uint64_t seed, double x, double y)
{
	const double cellX     = std::floor(x);
	const double cellY     = std::floor(y);
	const int64_t indexX   = static_cast<int64_t>(cellX);
	const int64_t indexY   = static_cast<int64_t>(cellY);
	const double weightX   = SmoothStep(x - cellX);
	const double weightY   = SmoothStep(y - cellY);
	const double corner00  = HashSigned(seed, indexX, indexY);
	const double corner10  = HashSigned(seed, indexX + 1, indexY);
	const double corner01  = HashSigned(seed, indexX, indexY + 1);
	const double corner11  = HashSigned(seed, indexX + 1, indexY + 1);
	const double bottom    = corner00 + (corner10 - corner00) * weightX;
	const double top       = corner01 + (corner11 - corner01) * weightX;
	return bottom + (top - bottom) * weightY;
}

double Fbm1D(uint64_t seed, double x, int octaves)
{
	double sum       = 0.0;
	double amplitude = 1.0;
	double total     = 0.0;
	double frequency = 1.0;
	for (int octave = 0; octave < octaves; ++octave)
	{
		sum += ValueNoise1D(seed + static_cast<uint64_t>(octave) * 7919u, x * frequency) * amplitude;
		total += amplitude;
		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return total > 0.0 ? sum / total : 0.0;
}

double Fbm2D(uint64_t seed, double x, double y, int octaves)
{
	double sum       = 0.0;
	double amplitude = 1.0;
	double total     = 0.0;
	double frequency = 1.0;
	for (int octave = 0; octave < octaves; ++octave)
	{
		sum += ValueNoise2D(seed + static_cast<uint64_t>(octave) * 7919u, x * frequency, y * frequency) * amplitude;
		total += amplitude;
		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return total > 0.0 ? sum / total : 0.0;
}

Random::Random(uint64_t seed)
    : m_state(0),
      m_increment((Hash64(seed ^ 0x5851F42D4C957F2Dull) << 1u) | 1u)
{
	NextU32();
	m_state += Hash64(seed);
	NextU32();
}

uint32_t Random::NextU32()
{
	const uint64_t previous = m_state;
	m_state                 = previous * 6364136223846793005ull + m_increment;
	const uint32_t xorshifted = static_cast<uint32_t>(((previous >> 18u) ^ previous) >> 27u);
	const uint32_t rotation   = static_cast<uint32_t>(previous >> 59u);
	return (xorshifted >> rotation) | (xorshifted << ((32u - rotation) & 31u));
}

double Random::NextUnit()
{
	return static_cast<double>(NextU32()) * (1.0 / 4294967296.0);
}

double Random::Range(double minimum, double maximum)
{
	return minimum + (maximum - minimum) * NextUnit();
}
}  // namespace Struktur::RideWorld
