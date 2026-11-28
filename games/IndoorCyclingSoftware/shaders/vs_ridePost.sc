$input a_position, a_texcoord0
$output v_texcoord0

// Upscales the low-resolution scene to the screen (orthographic pixel-space quad).
#include <bgfx_shader.sh>

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position.xy, 0.0, 1.0));
	v_texcoord0 = vec4(a_texcoord0.xy, 0.0, 0.0);
}
