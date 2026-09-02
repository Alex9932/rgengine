#version 430 core

#define NORMALMAP 1

layout(location = 0) in vec3 o_position;
layout(location = 1) in vec3 o_T;
layout(location = 2) in vec3 o_B;
layout(location = 3) in vec3 o_N;
layout(location = 4) in vec2 o_uv;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 wpos;

layout(set = 1, binding = 0) uniform texture2D t_albedo;
layout(set = 1, binding = 1) uniform texture2D t_normal;
layout(set = 1, binding = 2) uniform texture2D t_pbr;

layout(set = 2, binding = 0) uniform sampler smplr;

layout(push_constant) uniform PushConstants {
	mat4 model;
    vec4 color;
} push;

void main() {
	//vec2 uv = vec2(o_uv.x, -o_uv.y);
	vec2 uv = o_uv;

	vec4 t_col4 = texture(sampler2D(t_albedo, smplr), uv);
	vec3 t_col  = t_col4.rgb;
	vec3 t_norm = texture(sampler2D(t_normal, smplr), uv).rgb;
	vec3 t_pbr  = texture(sampler2D(t_pbr,    smplr), uv).rgb;

	if (t_col4.a < 0.3) {
		discard;
	}

	vec3 N = vec3(0);

#if NORMALMAP
	mat3 TBN = mat3(normalize(o_T), normalize(o_B), normalize(o_N));
	vec3 nmap = t_norm * 2.0 - 1.0;
	N = normalize(TBN * nmap);
#else
	N = normalize(o_N);
#endif

	color.rgb = t_col * push.color.rgb;
	//color.a = t_col4.a;//t_pbr.x;
	color.a = t_pbr.x;
	normal.xyz = N;
	normal.a = t_pbr.y;
	wpos.xyz = o_position;
	wpos.a = t_pbr.z;
}