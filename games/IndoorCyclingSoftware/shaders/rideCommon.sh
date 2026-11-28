// Shared by the ride world shaders (vs_/fs_rideTerrain, rideRoad, rideProp). Uniforms are set by
// Engine/Renderer/RideWorldRenderer.cpp.
#ifndef RIDE_COMMON_SH
#define RIDE_COMMON_SH

// x: height texture spacing (m), y: texture width (texels), z: road-height scale (1 = add the road height), w: time (s)
uniform vec4 u_heightParams;
// x: distance from the height texture's first texel to this chunk's start (m), y: stripe offset (m),
// z, w: chunk origin modulo 1024 (m) for world-stable texture coordinates
uniform vec4 u_chunk;
uniform vec4 u_cameraPos;  // xyz: camera position in the render frame (chunks are translated relative to it)
uniform vec4 u_fogColor;   // rgb, a: density (per metre)
uniform vec4 u_fogParams;  // x: start (m), y: bands (0 = smooth), z: max fog, w: unused
uniform vec4 u_sunDir;     // xyz: towards the sun, w: sun strength
uniform vec4 u_sunColor;
uniform vec4 u_ambient;

#if BGFX_SHADER_TYPE_VERTEX
SAMPLER2D(s_height, 0);

// Road height relative to the rider at a distance (m) from the height texture's first texel. Point-sampled and
// interpolated by hand: float textures aren't filterable everywhere.
float rideRoadHeight(float distance)
{
	float texel  = max(distance / u_heightParams.x, 0.0);
	float first  = floor(texel);
	float weight = texel - first;
	float width  = u_heightParams.y;
	float lower = texture2DLod(s_height, vec2((first + 0.5) / width, 0.5), 0.0).x;
	float upper = texture2DLod(s_height, vec2((first + 1.5) / width, 0.5), 0.0).x;
	return mix(lower, upper, weight) * u_heightParams.z;
}
#endif

vec3 rideApplyFog(vec3 color, float distance)
{
	float amount = 1.0 - exp(-u_fogColor.a * max(distance - u_fogParams.x, 0.0));
	if (u_fogParams.y > 0.5)
	{
		// Banded (retro): quantise the fog amount.
		amount = floor(amount * u_fogParams.y + 0.5) / u_fogParams.y;
	}
	amount = min(amount, u_fogParams.z);
	return mix(color, u_fogColor.rgb, amount);
}

vec3 rideLight(vec3 normal)
{
	float diffuse = max(dot(normalize(normal), u_sunDir.xyz), 0.0) * u_sunDir.w;
	return u_ambient.rgb + u_sunColor.rgb * diffuse;
}

#endif
