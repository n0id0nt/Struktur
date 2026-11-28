$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_stars, 0);
SAMPLER2D(s_mountainsFar, 1);
SAMPLER2D(s_mountainsNear, 2);

uniform mat4 u_skyInvViewProj;  // inverse of the rotation-only view-projection
uniform vec4 u_skyZenith;
uniform vec4 u_skyHorizon;
uniform vec4 u_skyGround;
uniform vec4 u_skySun;          // xyz: towards the sun, w: halo strength
uniform vec4 u_skySunColor;
uniform vec4 u_skyParams;       // x: stars, y: mountain height (radians), z: far parallax, w: near parallax
uniform vec4 u_skyMountainFar;  // rgb (already fogged)
uniform vec4 u_skyMountainNear;

void main()
{
	vec4 farPoint = mul(u_skyInvViewProj, vec4(v_texcoord0.xy, 1.0, 1.0));
	vec3 ray      = normalize(farPoint.xyz / farPoint.w);
	float elevation = asin(clamp(ray.y, -1.0, 1.0));
	float heading   = atan2(ray.z, ray.x);
	float around    = heading / 6.2831853;

	vec3 color;
	if (ray.y >= 0.0)
	{
		color = mix(u_skyHorizon.rgb, u_skyZenith.rgb, pow(clamp(ray.y, 0.0, 1.0), 0.55));
		// Stars fade in above the horizon haze.
		float stars = texture2D(s_stars, vec2(around * 3.0, elevation * 1.6)).a;
		color += vec3(stars, stars, stars) * u_skyParams.x * smoothstep(0.02, 0.2, ray.y);
	}
	else
	{
		color = mix(u_skyHorizon.rgb, u_skyGround.rgb, clamp(-ray.y * 6.0, 0.0, 1.0));
	}

	// Sun halo (the disc itself is a sprite drawn after this).
	float facing = max(dot(ray, u_skySun.xyz), 0.0);
	color += u_skySunColor.rgb * (pow(facing, 24.0) * 0.35 + pow(facing, 300.0) * 0.6) * u_skySun.w;

	// Two parallax mountain bands just above the horizon.
	float bandFar = u_skyParams.y;
	if (bandFar > 0.0 && elevation > -0.02 && elevation < bandFar)
	{
		float alpha = texture2D(s_mountainsFar, vec2(around * 2.0 + u_skyParams.z, 1.0 - max(elevation, 0.0) / bandFar)).a;
		color = mix(color, u_skyMountainFar.rgb, alpha);
	}
	float bandNear = bandFar * 0.55;
	if (bandNear > 0.0 && elevation > -0.02 && elevation < bandNear)
	{
		float alpha = texture2D(s_mountainsNear, vec2(around * 3.0 + u_skyParams.w, 1.0 - max(elevation, 0.0) / bandNear)).a;
		color = mix(color, u_skyMountainNear.rgb, alpha);
	}
	gl_FragColor = vec4(color, 1.0);
}
