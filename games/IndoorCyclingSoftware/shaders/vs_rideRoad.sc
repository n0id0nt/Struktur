$input a_position, a_normal, a_color0, a_texcoord0
$output v_color0, v_normal, v_texcoord0, v_texcoord1

// The road ribbon: same placement as the terrain; the stripe pattern is drawn in fs_rideRoad from (distance, lateral).
#include <bgfx_shader.sh>
#include "rideCommon.sh"

void main()
{
	vec3 position = a_position;
	position.y += rideRoadHeight(a_texcoord0.x + u_chunk.x);
	vec4 world = mul(u_model[0], vec4(position, 1.0));
	gl_Position = mul(u_viewProj, world);
	v_color0    = a_color0;
	v_normal    = a_normal;
	v_texcoord0 = vec4(a_texcoord0.x + u_chunk.y, a_texcoord0.y, 0.0, 0.0);
	v_texcoord1 = vec4(world.xyz - u_cameraPos.xyz, 0.0);
}
