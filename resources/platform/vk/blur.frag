#version 430 core

layout(location = 0) in vec2 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;

layout(push_constant) uniform PushConstants {
    layout(offset = 128) vec4 axis; // XY axis for blur, ZW - screen size
} push;

#define RADIUS 7
#define THRESHOLD 0.8

void main() {

	vec2 uv = o_position * 0.5 + 0.5;

	vec2 ax = normalize(push.axis.xy); // Blur axis
	vec2 ss = push.axis.zw; // Screen size
	vec2 ts = 1.0 / ss;     // Texel size

	vec4 bColor = vec4(0.0);

	for (int i = -RADIUS; i <= RADIUS; i++) {
		float weight = exp(-(i*i)/8.9) * 0.1974;
		vec2 offset = ax * ts * float(i);
		vec4 tc = texture(sampler2D(t_unit0, smplr), uv + offset);
		
		if(push.axis.x > 1) {
			// If x axis is greater than 1, we are doing a apply threshold
			if(tc.r < THRESHOLD && tc.g < THRESHOLD && tc.b < THRESHOLD) {
				weight = 0;
			}
		}

		bColor += tc * weight;
	}

	color.rgb = bColor.rgb;
	color.a = 1;
}