#version 430 core

layout(location = 0) in vec3 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;
layout(set = 1, binding = 1) uniform texture2D t_unit1;
layout(set = 1, binding = 2) uniform texture2D t_unit2;

void main() {

	vec2 uv = o_position.xy * 0.5 + 0.5;
	uv.y = 1.0 - uv.y;
	
	vec4 u0 = texture(sampler2D(t_unit0, smplr), uv);
	vec4 u1 = texture(sampler2D(t_unit1, smplr), uv);
	vec4 u2 = texture(sampler2D(t_unit2, smplr), uv);

	vec3 C = u0.rgb;
	vec3 N = normalize(u1.xyz);
	vec3 P = u2.xyz;
	
	
	float light = 0.2;
	
	vec3 L = vec3(0, 1, -1);
	float global = max(0, dot(normalize(N), normalize(L)));

	light += global;


	color.rgb = C * light;
	color.a = 1;
}