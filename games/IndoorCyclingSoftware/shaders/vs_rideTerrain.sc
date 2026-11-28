$input a_position, a_normal, a_color0, a_texcoord0
$output v_color0, v_normal, v_texcoord0, v_texcoord1

// Terrain and water: positions are relative to the chunk origin with heights relative to the road; the road's own
// elevation comes from the height texture, so the profile ahead can change after the chunk was built.
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
	// Detail texture coordinates in world metres (chunk origin folded modulo 1024 m keeps them precise).
	v_texcoord0 = vec4((a_position.x + u_chunk.z) * 0.25, (a_position.z + u_chunk.w) * 0.25, a_texcoord0.z, a_texcoord0.y);
	v_texcoord1 = vec4(world.xyz - u_cameraPos.xyz, a_texcoord0.x);
}
