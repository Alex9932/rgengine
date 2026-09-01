#version 430 core

layout(location = 0) in vec2 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;
layout(set = 2, binding = 0) uniform texture2D t_unit1;
layout(set = 3, binding = 0) uniform texture2D t_unit2;
layout(set = 4, binding = 0) uniform texture2D t_unit3;

vec3 toneMapACES(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 saturateColor(vec3 color, float saturation) {
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(luma), color, saturation);
}

void main() {

	vec2 uv = o_position * 0.5 + 0.5;
	uv.y = 1.0 - uv.y; // Flip final image
	
	vec4 u0 = texture(sampler2D(t_unit0, smplr), uv);
	vec4 u1 = texture(sampler2D(t_unit1, smplr), uv); // Blur
	vec4 u2 = texture(sampler2D(t_unit2, smplr), uv); // ssgi
	vec4 u3 = texture(sampler2D(t_unit3, smplr), uv); // ssr

	vec3 lColor = vec3(0);
//	lColor = pow(u0.rgb, vec3(1.0 / 2.2)); // And apply gamma correction
	lColor = u0.rgb;
	lColor += u3.rgb; // Add reflection
	lColor += u1.rgb; // Add blurred image on top

//	lColor = u3.rgb;

	color.rgb = lColor;//saturateColor(toneMapACES(lColor), 1.02);
	color.a = 1;
}