vec4 v_color0    : COLOR0    = vec4(1.0, 1.0, 1.0, 1.0);
vec3 v_normal    : NORMAL    = vec3(0.0, 1.0, 0.0);
vec4 v_texcoord0 : TEXCOORD0 = vec4(0.0, 0.0, 0.0, 0.0);
vec4 v_texcoord1 : TEXCOORD1 = vec4(0.0, 0.0, 0.0, 0.0);

vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec4 a_color0    : COLOR0;
vec3 a_texcoord0 : TEXCOORD0;
// Prop instance data (Engine/RideWorld/ChunkBuilder.h PropInstance): x, y, z, distance / height, angle, variant, tint.
// This bgfx feeds instance data from TEXCOORD31 downwards (BGFX_CONFIG_INSTANCE_DATA_FIRST_TEXCOORD), not 7.
vec4 i_data0     : TEXCOORD31;
vec4 i_data1     : TEXCOORD30;
