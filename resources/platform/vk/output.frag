#version 430 core

layout(location = 0) in vec2 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;

void main() {
	vec2 uv = o_position * 0.5 + 0.5;
	color.rgb = texture(sampler2D(t_unit0, smplr), uv).rgb;
	color.a = 1;
}