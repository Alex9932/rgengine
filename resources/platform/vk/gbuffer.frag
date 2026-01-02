#version 430 core

layout(location = 0) in vec3 o_position;
layout(location = 1) in vec3 o_normal;
layout(location = 2) in vec2 o_uv;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 wpos;

layout(set = 1, binding = 0) uniform texture2D t_albedo;
layout(set = 1, binding = 1) uniform texture2D t_normal;
layout(set = 1, binding = 2) uniform texture2D t_pbr;

layout(set = 2, binding = 0) uniform sampler smplr;

layout(push_constant) uniform PushConstants {
    layout(offset = 128) vec4 color;
} push;

void main() {
	//vec2 uv = vec2(o_uv.x, -o_uv.y);
	vec2 uv = o_uv;

	color.rgb = texture(sampler2D(t_albedo, smplr), uv).rgb * push.color.rgb;
	color.a = 1;

	normal.xyz = normalize(o_normal);
	wpos.xyz   = o_position;
}