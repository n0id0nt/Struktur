$input a_position, a_texcoord0, a_color0, a_texcoord1
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

// Packed vec4(x, 0, 0, 0) uniforms - one bgfx uniform per tunable param, same convention
// vs_soulEffect.sc/fs_soulEffect.sc already use (see ShaderSystem::ApplyUniforms). time is driven by
// UIRenderer::Flush() each frame (see its own comment) using GameContext's TimeSystem::scaledTime, matching
// UIRichLabel's reveal-effect time source from Phase 6.
uniform vec4 time;
uniform vec4 waveAmplitude;
uniform vec4 waveFrequency;
uniform vec4 shakeRate;
uniform vec4 shakeLevel;
uniform vec4 pulseFrequency;
uniform vec4 rainbowFrequency;
uniform vec4 rainbowSaturation;
uniform vec4 rainbowValue;
uniform vec4 tornadoRadius;
uniform vec4 tornadoFrequency;
uniform vec4 fadeStart;
uniform vec4 fadeLength;

// Classic cheap GLSL pseudo-random hash (sin/fract-based) - portable across every profile this project
// targets, unlike a true noise texture or bitwise-hash approach. Building block for gradientNoise1 below
// (deterministic per input, reproducible, no per-frame CPU-side random-number generation needed).
float hash(float n)
{
	return fract(sin(n) * 43758.5453123);
}

// 1D gradient ("Perlin-style") noise - same smoothstep-eased, lerp-between-lattice-points shape as
// Util::Noise::PerlinNoise1 (see CameraSystem::CalculateCameraShake, the screen-shake feature this mirrors),
// just built on the hash() function above instead of a CPU-side permutation table: a GPU vertex shader can't
// cheaply build/index a 512-entry table per invocation the way that CPU code does once per frame, and a large
// const array wouldn't be portable across every profile this project targets either. `seed` distinguishes
// independent noise fields sampled at the same `value` (e.g. one per axis, or per character) the same way
// PerlinNoise1's own seed parameter does - offset into hash()'s input rather than a literal RNG seed.
float gradientNoise1(float seed, float value)
{
	float cell = floor(value);
	float frac = value - cell;

	// Smoothstep easing (3t^2 - 2t^3) - identical curve to Util::Noise::Smoothstep, so interpolation between
	// lattice points has the same ease-in/ease-out shape the camera shake already uses.
	float u = frac * frac * (3.0 - 2.0 * frac);

	// Per-lattice-point signed gradient magnitude from the hash, in [-1, 1) - the 1D analogue of Grad()'s
	// hash-driven magnitude (a 1D "gradient" has no direction to dot against, just a signed scale).
	float gradA = hash(seed + cell) * 2.0 - 1.0;
	float gradB = hash(seed + cell + 1.0) * 2.0 - 1.0;

	return mix(gradA * frac, gradB * (frac - 1.0), u);
}

// Standard compact HSV->RGB (h/s/v all 0..1) - used by the rainbow effect below. No portability concerns
// versus a bitwise/integer approach, unlike the effectMask/charIndex decode above - this is pure float math.
vec3 hsv2rgb(vec3 c)
{
	vec4 k = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + k.xyz) * 6.0 - k.www);
	return c.z * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), c.y);
}

void main()
{
	// a_texcoord1 arrives NORMALIZED (0.0-1.0 range, real UNORM hardware conversion - see
	// AnimQuadVertex::BuildAnimQuadVertexLayout's own comment for why this can't be an unnormalized fetch on
	// Direct3D: there's no vertex format there for "unnormalized uint8 numerically converted to float", only
	// UNORM or a true integer fetch, and the integer path needs bgfx's asInt (unsupported on GLES2/the "100_es"
	// web profile this project also targets). Recover the original 0-255 byte values by multiplying back up,
	// snapping to the nearest integer to remove the tiny (~1e-5) UNORM round-trip error before it can perturb
	// the mod()/floor() bit tests below.
	float effectMaskByte    = floor(a_texcoord1.x * 255.0 + 0.5);
	float charIndexLowByte  = floor(a_texcoord1.y * 255.0 + 0.5);
	float charIndexHighByte = floor(a_texcoord1.z * 255.0 + 0.5);
	float effectMask = effectMaskByte;
	float charIndex  = charIndexHighByte * 256.0 + charIndexLowByte;

	// Bit tests via floor/mod rather than bitwise ops, for the same GLES2/old-HLSL-profile portability reason -
	// effectMask is always a small (0-63) integer-valued float, so this is exact, not approximate. Bit order
	// matches Text::RichRun::effectMask/AnimationFlags exactly: wave, shake, pulse, rainbow, tornado, fade.
	bool hasWave     = mod(effectMask, 2.0) >= 1.0;
	bool hasShake    = mod(floor(effectMask / 2.0), 2.0) >= 1.0;
	bool hasPulse    = mod(floor(effectMask / 4.0), 2.0) >= 1.0;
	bool hasRainbow  = mod(floor(effectMask / 8.0), 2.0) >= 1.0;
	bool hasTornado  = mod(floor(effectMask / 16.0), 2.0) >= 1.0;
	bool hasFade     = mod(floor(effectMask / 32.0), 2.0) >= 1.0;

	vec3 displaced = a_position;
	vec4 color     = a_color0;

	if (hasWave)
	{
		// Phase offset per character (0.5 rad/char) plus a time-driven term - gives the classic "ripple runs
		// across the word" look rather than every character bobbing in lockstep.
		displaced.y += sin(charIndex * 0.5 + time.x * waveFrequency.x) * waveAmplitude.x;
	}

	if (hasShake)
	{
		// Smooth, continuous noise-driven wobble (gradientNoise1 above) rather than snapping to a new random
		// offset shakeRate times per second - the same continuous-Perlin-field feel Component::Camera's own
		// screen shake has (see CameraSystem::CalculateCameraShake), instead of the old sin/fract per-step
		// jitter this replaced. shakeRate now controls how fast the noise field is traversed (higher = faster
		// wobble) rather than a discrete update count - conceptually the same "shake speed" knob as before, see
		// Text::ParseMarkup's [shake rate=...] tag, just driving a continuous domain instead of a step counter.
		// x/y each sample a different point along the field (charIndex offsets which "lane", the +100 constant
		// offsets y from x) so a glyph's wobble isn't a single oscillating line, and every character's own
		// lane is independent rather than shaking in lockstep.
		float shakeTime = time.x * shakeRate.x;
		displaced.x += gradientNoise1(charIndex * 0.5, shakeTime) * shakeLevel.x;
		displaced.y += gradientNoise1(charIndex * 0.5 + 100.0, shakeTime) * shakeLevel.x;
	}

	if (hasTornado)
	{
		// Each character orbits in a circle, phase-offset by charIndex so the orbit position sweeps across
		// the word rather than every character moving in lockstep - same "ripple via phase offset" idea wave
		// uses, just applied to both axes at once instead of one.
		float tornadoAngle = charIndex * 0.5 + time.x * tornadoFrequency.x;
		displaced.x += cos(tornadoAngle) * tornadoRadius.x;
		displaced.y += sin(tornadoAngle) * tornadoRadius.x;
	}

	if (hasPulse)
	{
		// Alpha-only oscillation (no color-blend target, see the design doc's v1 scoping note) - never fully
		// invisible (floor of 0.4) so pulsing text stays legible at its dimmest.
		float pulse = sin(time.x * pulseFrequency.x) * 0.5 + 0.5;
		color.a *= mix(0.4, 1.0, pulse);
	}

	if (hasRainbow)
	{
		// Replaces rgb (not multiplies - a cycling hue wouldn't be visible multiplied against an already-set
		// tint colour) while preserving alpha, so rainbow still composes with pulse's alpha modulation above
		// and fade's below regardless of tag nesting order.
		float hue     = fract(charIndex * 0.1 + time.x * rainbowFrequency.x);
		color.rgb     = hsv2rgb(vec3(hue, rainbowSaturation.x, rainbowValue.x));
	}

	if (hasFade)
	{
		// Static (no time term at all) per-character opacity gradient by reading-order position - characters
		// before fadeStart stay opaque, fadeStart..fadeStart+fadeLength ramps linearly to fully transparent.
		float t = clamp((charIndex - fadeStart.x) / max(fadeLength.x, 0.001), 0.0, 1.0);
		color.a *= (1.0 - t);
	}

	gl_Position = mul(u_modelViewProj, vec4(displaced, 1.0));
	v_texcoord0 = a_texcoord0;
	v_color0    = color;
}
