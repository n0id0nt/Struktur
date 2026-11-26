$input a_position, a_texcoord0, i_data0, i_data1
$output v_color0, v_texcoord0, v_texcoord1

// Instanced props: one quad per instance, either camera-facing (cylindrical billboard) or aligned with the road.
// i_data0 = position (chunk-relative, y relative to the road) + distance from the chunk start;
// i_data1 = height (m), angle (rad), variant (0-1, picks a column), tint (0-1).
#include <bgfx_shader.sh>
#include "rideCommon.sh"

uniform vec4 u_propSheet;  // x: columns, y: rows, z: row, w: frame aspect (width / height)
uniform vec4 u_propMode;   // x: 1 = aligned to the road, y: tint strength, z: unused, w: alpha test threshold (fragment)
uniform vec4 u_cameraRight;  // xyz: camera right, horizontal

void main()
{
	float height = i_data1.x;
	float width  = height * u_propSheet.w;
	vec3 right   = u_cameraRight.xyz;
	if (u_propMode.x > 0.5)
	{
		right = vec3(cos(i_data1.y), 0.0, sin(i_data1.y));
	}
	vec3 position = i_data0.xyz + right * (a_position.x * width) + vec3(0.0, a_position.y * height, 0.0);
	position.y += rideRoadHeight(i_data0.w + u_chunk.x);
	vec4 world  = mul(u_model[0], vec4(position, 1.0));
	gl_Position = mul(u_viewProj, world);

	float column = min(floor(i_data1.z * u_propSheet.x), u_propSheet.x - 1.0);
	v_texcoord0  = vec4((column + a_texcoord0.x) / u_propSheet.x, (u_propSheet.z + a_texcoord0.y) / u_propSheet.y, 0.0, 0.0);
	float shade  = 1.0 + (i_data1.w - 0.5) * u_propMode.y;
	v_color0     = vec4(shade, shade, shade, 1.0);
	v_texcoord1  = vec4(world.xyz - u_cameraPos.xyz, 0.0);
}
