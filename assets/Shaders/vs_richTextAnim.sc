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
// targets, unlike a true noise texture or bitwise-hash approach. Used by the shake effect below so each
// glyph's jitter is deterministic per (charIndex, time-step) rather than truly random - reproducible, no
// per-frame CPU-side random-number generation needed.
float hash(float n)
{
	return fract(sin(n) * 43758.5453123);
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
	// a_texcoord1 arrives as raw 0..255 floats (unnormalized uint8 vertex fetch - see
	// AnimQuadVertex::BuildAnimQuadVertexLayout), not 0..1 normalized color-style values. Reconstructing
	// charIndex from two bytes (rather than relying on a single wide integer attribute) avoids any reliance on
	// bgfx's asInt integer vertex fetch, which isn't supported on every profile this project targets (notably
	// GLES2/the "100_es" web profile) - see AnimQuadVertex.h's own comment.
	float effectMask = a_texcoord1.x;
	float charIndex  = a_texcoord1.z * 256.0 + a_texcoord1.y;

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
		// floor(time * rate) steps to a new pseudo-random offset `rate` times per second (matching Godot's own
		// "rate = updates per second" semantics for [shake]) - charIndex offsets the hash seed so every
		// character shakes independently rather than in unison.
		float shakeSeed = floor(time.x * shakeRate.x) + charIndex * 13.37;
		displaced.x += (hash(shakeSeed) - 0.5) * 2.0 * shakeLevel.x;
		displaced.y += (hash(shakeSeed + 91.7) - 0.5) * 2.0 * shakeLevel.x;
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
