$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>
#include "rideCommon.sh"

SAMPLER2D(s_texColor, 1);

uniform vec4 u_propTint;  // rgb multiplier (actors: jersey tint), a: 1 = lit by the sun/ambient
uniform vec4 u_propMode;  // w: alpha test threshold

void main()
{
	vec4 texel = texture2D(s_texColor, v_texcoord0.xy);
	// Alpha test instead of blending: props needn't be sorted and keep their crisp pixel edges.
	if (texel.a < u_propMode.w)
	{
		discard;
	}
	vec3 light = mix(vec3(1.0, 1.0, 1.0), u_ambient.rgb + u_sunColor.rgb * (0.6 * u_sunDir.w), u_propTint.a);
	vec3 color = texel.rgb * v_color0.rgb * u_propTint.rgb * light;
	gl_FragColor = vec4(rideApplyFog(color, length(v_texcoord1.xyz)), 1.0);
}
