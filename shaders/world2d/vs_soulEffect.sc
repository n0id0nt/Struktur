$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

// Packed vec4(x, 0, 0, 0) uniforms - one bgfx uniform per original raylib uniform name (see ShaderResource /
// ShaderSystem::ApplyUniforms), so Wren's existing setFloatUniform("waveAmplitude", ...) calls need no changes.
uniform vec4 waveAmplitude;
uniform vec4 waveFrequency;
uniform vec4 waveSpeed;
uniform vec4 waveDirection;
uniform vec4 time;

void main()
{
	vec2 worldPos = a_position.xy;
	vec2 dir      = waveDirection.xy;
	float t       = time.x;
	float amp     = waveAmplitude.x;
	float freq    = waveFrequency.x;
	float speed   = waveSpeed.x;

	float wave1 = sin(dot(worldPos, dir) * freq + t * speed) * amp;
	float wave2 = sin(dot(worldPos, vec2(-dir.y, dir.x)) * freq * 1.3 + t * speed * 0.7) * amp * 0.5;
	float wave3 = sin(length(worldPos - vec2(0.5, 0.5)) * freq * 2.0 - t * speed * 1.5) * amp * 0.3;
	float totalWave = wave1 + wave2 + wave3;

	vec3 displacedPosition = a_position;
	displacedPosition.x += totalWave * 0.5;
	displacedPosition.y += totalWave * 0.3;

	gl_Position = mul(u_modelViewProj, vec4(displacedPosition, 1.0));
	v_texcoord0 = a_texcoord0;
	v_color0    = a_color0;
}
