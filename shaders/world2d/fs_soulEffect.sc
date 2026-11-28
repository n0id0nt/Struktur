$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

// Packed vec4(x, 0, 0, 0) uniforms (vec4(xyz, 0) for soulColor) - one bgfx uniform per original raylib uniform
// name (see ShaderResource / ShaderSystem::ApplyUniforms), so Wren's existing setFloatUniform(...)/
// setVec3Uniform(...) calls need no changes. time/resolution are auto-injected by ApplyUniforms every frame.
uniform vec4 time;
uniform vec4 resolution;
uniform vec4 soulColor;
uniform vec4 glowIntensity;
uniform vec4 rippleSpeed;
uniform vec4 rippleFrequency;
uniform vec4 scanlineIntensity;
uniform vec4 chromaticAberration;
uniform vec4 glitchFrequency;
uniform vec4 holographicShift;

void main()
{
	vec2 uv = v_texcoord0;

	float scanlines = sin(uv.y * resolution.y) * scanlineIntensity.x;
	scanlines        = scanlines * 0.5 + (1.0 - scanlineIntensity.x * 0.5);

	float aberration = chromaticAberration.x * sin(time.x * rippleSpeed.x + uv.y * rippleFrequency.x);
	vec2 redOffset    = vec2(aberration, 0.0);
	vec2 blueOffset   = vec2(-aberration, 0.0);

	float r     = texture2D(s_texColor, uv + redOffset).r;
	float g     = texture2D(s_texColor, uv).g;
	float b     = texture2D(s_texColor, uv + blueOffset).b;
	float alpha = texture2D(s_texColor, uv).a;

	vec3 baseColor = vec3(r, g, b);

	float glitch = step(1.0 - glitchFrequency.x, sin(time.x * 10.0 + uv.y * 20.0));
	baseColor    = mix(baseColor, baseColor * soulColor.xyz, glitch * 0.5);

	float holo      = sin(time.x * 2.0 + uv.y * holographicShift.x) * 0.5 + 0.5;
	vec3 holoColor  = mix(vec3(0.5, 1.0, 1.0), vec3(1.0, 0.5, 1.0), holo);
	baseColor       = mix(baseColor, baseColor * holoColor, 0.3);

	baseColor *= scanlines;

	vec2 center = vec2(0.5, 0.5);
	float dist  = distance(uv, center);
	float glow  = (1.0 - smoothstep(0.0, 0.8, dist)) * glowIntensity.x;
	baseColor += soulColor.xyz * glow * 0.2;

	gl_FragColor = vec4(baseColor, alpha) * v_color0;
}
