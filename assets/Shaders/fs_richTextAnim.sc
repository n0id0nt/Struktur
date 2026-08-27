$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// Identical to fs_spriteCoverage.sc - animated glyphs sample the same single-channel (R8) font atlas as
// regular text, just with a different vertex shader upstream (vs_richTextAnim.sc) doing the per-character
// displacement/alpha work. Kept as its own file rather than sharing fs_spriteCoverage.sc directly, matching
// this project's existing convention of one file per embedded program name (see vs_spriteCoverage.sc, which
// is likewise a content-identical copy of vs_sprite.sc for the same reason).
SAMPLER2D(s_texColor, 0);

void main()
{
	float coverage = texture2D(s_texColor, v_texcoord0).r;
	gl_FragColor = vec4(coverage, coverage, coverage, coverage) * v_color0;
}
