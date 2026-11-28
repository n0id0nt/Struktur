$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// For single-channel (R8) atlases, e.g. FontResource's glyph atlas - the texture stores only per-texel
// coverage, not color, so the sampled value is broadcast into rgb and used as alpha too, then tinted by
// vertex color. Mirrors fs_sprite.sc's texture.rgba * v_color0 but for atlases with no color channels of
// their own (see FontResource::LoadToGpu).
SAMPLER2D(s_texColor, 0);

void main()
{
	float coverage = texture2D(s_texColor, v_texcoord0).r;
	gl_FragColor = vec4(coverage, coverage, coverage, coverage) * v_color0;
}
