#version 430 core

layout(location = 0) in vec2 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;
layout(set = 2, binding = 0) uniform texture2D t_unit1;

void main() {

	vec2 uv = o_position * 0.5 + 0.5;
	uv.y = 1.0 - uv.y; // Flip final image
	
	vec4 u0 = texture(sampler2D(t_unit0, smplr), uv);
	vec4 u1 = texture(sampler2D(t_unit1, smplr), uv); // Blur

	vec3 lColor;
	//lColor = pow(u0.rgb, vec3(1.0 / 2.2)); // And apply gamma correction
	lColor = u0.rgb;
	lColor += u1.rgb; // Add blurred image on top

	color.rgb = lColor;
	color.a = 1;
}