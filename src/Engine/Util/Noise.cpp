#include "Noise.h"

#include <stdlib.h>

#include <algorithm>
#include <cmath>

#include "Engine/Util/MathUtil.h"

double Struktur::Util::Noise::Grad(int hash, double value)
{
	int gradientIndex = hash & 15;
	double grad       = 1 + (gradientIndex & 7);  // Gradient value 1-8
	if (gradientIndex & 8)
	{
		grad = -grad;  // Randomly invert half of them
	}
	return grad * value;
}

double Struktur::Util::Noise::Smoothstep(double edge0, double edge1, double x)
{
	// Scale, bias and saturate x to 0..1 range
	x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	// Evaluate polynomial
	return x * x * (3 - 2 * x);
}

double Struktur::Util::Noise::PerlinNoise1(unsigned int seed, double value)
{
	int permutation[512];
	::srand(seed);
	for (int i = 0; i < 256; ++i)
	{
		permutation[i] = permutation[256 + i] = rand() % 256;
	}

	int cell = static_cast<int>(std::floor(value)) & 255;
	value -= std::floor(value);

	double fade = Smoothstep(0.0, 1.0, value);

	int leftHash  = permutation[cell];
	int rightHash = permutation[cell + 1];

	return Util::Math::Lerp(fade, Grad(leftHash, value), Grad(rightHash, value - 1));
}
