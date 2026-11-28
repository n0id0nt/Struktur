#pragma once

#include <cstdint>

// Seeded, deterministic noise for the ride world. Integer hashing only (no std::rand, no global state), so the same
// seed and coordinates always give the same value on every platform that has IEEE doubles.
namespace Struktur::World
{
// 64-bit mix (SplitMix64 finaliser).
uint64_t Hash64(uint64_t value);
// Hash of a seed and up to three integer coordinates.
uint64_t Hash(uint64_t seed, int64_t x, int64_t y = 0, int64_t z = 0);
// Uniform double in [0, 1).
double HashUnit(uint64_t seed, int64_t x, int64_t y = 0, int64_t z = 0);
// Uniform double in [-1, 1).
double HashSigned(uint64_t seed, int64_t x, int64_t y = 0, int64_t z = 0);

// Smooth value noise in [-1, 1]. `x`/`y` are in lattice units (one lattice step per unit).
double ValueNoise1D(uint64_t seed, double x);
double ValueNoise2D(uint64_t seed, double x, double y);
// Fractal sum of `octaves` value-noise layers (each half the amplitude, twice the frequency), normalised to [-1, 1].
double Fbm1D(uint64_t seed, double x, int octaves);
double Fbm2D(uint64_t seed, double x, double y, int octaves);

// Small deterministic generator for placement loops (PCG32).
class Random
{
public:
	explicit Random(uint64_t seed);
	uint32_t NextU32();
	double NextUnit();                       // [0, 1)
	double Range(double minimum, double maximum);  // [minimum, maximum)

private:
	uint64_t m_state;
	uint64_t m_increment;
};
}  // namespace Struktur::World
