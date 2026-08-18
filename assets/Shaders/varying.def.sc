vec2 v_texcoord0 : TEXCOORD0 = vec2(0.0, 0.0);
vec4 v_color0 : COLOR0 = vec4(1.0, 1.0, 1.0, 1.0);

vec3 a_position : POSITION;
vec2 a_texcoord0 : TEXCOORD0;
vec4 a_color0 : COLOR0;
// Per-glyph animation data for vs_richTextAnim.sc only (see AnimQuadVertex) - raw 0-255 byte values, not a
// normalized color, hence its own texcoord slot rather than reusing a_color0/a_texcoord0. x=effectMask,
// y=charIndex low byte, z=charIndex high byte, w=reserved.
vec4 a_texcoord1 : TEXCOORD1;
