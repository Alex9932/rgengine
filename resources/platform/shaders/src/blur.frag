#version 430 core

layout(location = 0) in vec2 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;

layout(push_constant) uniform PushConstants {
    vec4 axis; // XY axis for blur, ZW - screen size
} push;

#define THRESHOLD 1.0
#define KNEE 0.3
#define SOFT_THRESHOLD 0.2

float getHighlights(vec3 color) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float soft = brightness - THRESHOLD + KNEE;
    soft = clamp(soft, 0.0, 2.0 * KNEE);
    soft = soft * soft / (4.0 * KNEE);
    float contribution = max(brightness - THRESHOLD, soft) / max(brightness, 0.0001);
    
    return contribution;
}

void main() {

	vec2 uv = o_position * 0.5 + 0.5;

	vec2 ax = normalize(push.axis.xy); // Blur axis
	vec2 ss = push.axis.zw; // Screen size
	vec2 ts = 1.0 / ss;     // Texel size

	vec4 bColor = vec4(0.0);

	for (int i = -7; i <= 7; i++) {
		float weight = exp(-(i*i)/8.9) * 0.1974;
		vec2 offset = ax * ts * float(i);
		vec4 tc = texture(sampler2D(t_unit0, smplr), uv + offset);
		
		if(push.axis.x > 1) {
			// If x axis is greater than 1, we are doing a apply threshold
			if(getHighlights(tc.rgb) < SOFT_THRESHOLD) {
				weight = 0;
			}
		}

		bColor += tc * weight;
	}

	color.rgb = bColor.rgb;
	color.a = 1;
}