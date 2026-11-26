$input a_position, a_texcoord0
$output v_texcoord0

// Full-screen sky: a quad in pixel space (the sky view is orthographic); the fragment shader rebuilds the view ray.
#include <bgfx_shader.sh>

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position.xy, 0.0, 1.0));
	// a_texcoord0.xy = normalised device coordinates of this corner.
	v_texcoord0 = vec4(a_texcoord0.xy, 0.0, 0.0);
}
