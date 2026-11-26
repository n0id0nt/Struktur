$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_scene, 0);

uniform vec4 u_post;   // x, y: scene size (pixels), z: palette levels per channel, w: 1 = quantise + dither
uniform vec4 u_post2;  // x: 1 = scanlines, y: scanline strength, z, w: unused

// 2x2 Bayer value for a cell (0..3): [[0, 2], [3, 1]].
float bayer2(vec2 cell)
{
	return mod(2.0 * cell.x + 3.0 * cell.y, 4.0);
}

void main()
{
	vec2 uv    = v_texcoord0.xy;
	vec3 color = texture2D(s_scene, uv).rgb;
	vec2 pixel = floor(uv * u_post.xy);

	if (u_post.w > 0.5)
	{
		// 4x4 ordered dither from two 2x2 levels, then quantise to u_post.z levels per channel.
		float threshold = (4.0 * bayer2(mod(pixel, 2.0)) + bayer2(mod(floor(pixel * 0.5), 2.0)) + 0.5) / 16.0;
		float levels    = max(u_post.z - 1.0, 1.0);
		color           = floor(color * levels + threshold) / levels;
	}
	if (u_post2.x > 0.5)
	{
		// Darken between the scene's own rows (CRT-style), whatever the upscale factor.
		float row  = fract(uv.y * u_post.y);
		float scan = 1.0 - u_post2.y * (1.0 - smoothstep(0.0, 0.35, row) * smoothstep(1.0, 0.65, row));
		color *= scan;
	}
	gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
