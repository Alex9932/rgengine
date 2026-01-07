#version 430 core

layout(location = 0) in vec3 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;

void main() {

	vec2 uv = o_position.xy * 0.5 + 0.5;
	uv.y = 1.0 - uv.y; // Flip final image
	
	vec4 u0 = texture(sampler2D(t_unit0, smplr), uv);

	//color.rgb = pow(u0.rgb, vec3(1.0 / 2.2)); // And apply gamma correction
	color.rgb = u0.rgb;
	color.a = 1;
}