$input v_color0, v_normal, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>
#include "rideCommon.sh"

SAMPLER2D(s_grass, 1);
SAMPLER2D(s_rock, 2);
SAMPLER2D(s_snow, 3);
SAMPLER2D(s_sand, 4);
SAMPLER2D(s_water, 5);

void main()
{
	vec2 uv        = v_texcoord0.xy;
	float material = floor(v_texcoord0.z + 0.5);
	vec3 detail;
	if (material < 0.5)
	{
		detail = texture2D(s_grass, uv).rgb;
	}
	else if (material < 1.5)
	{
		detail = texture2D(s_rock, uv).rgb;
	}
	else if (material < 2.5)
	{
		detail = texture2D(s_snow, uv).rgb;
	}
	else if (material < 3.5)
	{
		detail = texture2D(s_sand, uv).rgb;
	}
	else
	{
		// Water: slow scroll, a little brighter facing the sun.
		vec2 flow = uv * 0.5 + vec2(u_heightParams.w * 0.02, u_heightParams.w * 0.013);
		detail    = texture2D(s_water, flow).rgb * 1.1;
	}
	vec3 color    = v_color0.rgb * detail * rideLight(v_normal);
	float distance = length(v_texcoord1.xyz);
	gl_FragColor   = vec4(rideApplyFog(color, distance), 1.0);
}
