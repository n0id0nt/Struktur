$input v_color0, v_normal, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>
#include "rideCommon.sh"

SAMPLER2D(s_asphalt, 1);
SAMPLER2D(s_markings, 2);

uniform vec4 u_road;          // x: half width, y: shoulder, z: line width, w: rumble width (m)
uniform vec4 u_roadLine;      // centre / edge line colour
uniform vec4 u_roadShoulder;  // shoulder colour
uniform vec4 u_zoneColor;     // rumble strips: the rider's power zone colour

void main()
{
	float along    = v_texcoord0.x;  // metres along the road (offset keeps the dashes continuous)
	float lateral  = abs(v_texcoord0.y);
	float halfWidth = u_road.x;
	vec3 color = v_color0.rgb * texture2D(s_asphalt, vec2(v_texcoord0.y * 0.25, along * 0.25)).rgb;
	vec3 paint = u_roadLine.rgb * texture2D(s_markings, vec2(lateral * 4.0, along * 0.5)).rgb;

	if (lateral > halfWidth)
	{
		color = u_roadShoulder.rgb * texture2D(s_asphalt, vec2(lateral * 0.25, along * 0.5)).rgb;
		// Rumble strip just outside the edge: alternating zone colour and white blocks.
		if (lateral < halfWidth + u_road.w)
		{
			color = fract(along / 1.6) < 0.5 ? u_zoneColor.rgb : vec3(0.95, 0.95, 0.95);
		}
	}
	else if (lateral > halfWidth - u_road.z * 2.0 && lateral < halfWidth - u_road.z)
	{
		color = paint;  // solid edge line
	}
	else if (lateral < u_road.z * 0.5 && fract(along / 9.0) < 0.45)
	{
		color = paint;  // dashed centre line: 4 m dash, 5 m gap
	}

	color *= rideLight(v_normal);
	gl_FragColor = vec4(rideApplyFog(color, length(v_texcoord1.xyz)), 1.0);
}
