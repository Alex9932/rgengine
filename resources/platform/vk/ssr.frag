#version 430 core

layout(location = 0) in vec2 o_position;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0; // color
layout(set = 1, binding = 1) uniform texture2D t_unit1; // normal
layout(set = 1, binding = 2) uniform texture2D t_unit2; // wpos
layout(set = 1, binding = 3) uniform texture2D t_unit3; // depth
layout(set = 2, binding = 0) uniform texture2D t_unit4; // calculated light

layout(push_constant) uniform PushConstants {
    layout(offset = 128) vec4 camera;
	vec4 light_direction;
	vec4 light_color;
} push;

layout(set = 3, binding = 0) uniform UBO {
	mat4 proj;
	mat4 view;
	mat4 inv_proj;
	mat4 inv_view;
	vec4 cam_pos; // w - reserved
	vec4 cpu_rnd;
} ubo;

#define PI 3.14159265

float GetDepth(vec2 uv) {
	return texture(sampler2D(t_unit3, smplr), uv).r;
}

vec3 Project(vec4 P) {
	vec4 p = ubo.proj * ubo.view * P;
	return p.xyz / p.w;
}

// Return hit coord (or -1 if ray miss)
vec2 TraceRay(vec3 P, vec3 D) {
	
	float stepsize = 0.1;
	int   steps    = 300;
	float maxDist  = 50.0;

	vec3 step      = normalize(D) * stepsize;
	vec3 pos       = P;

	for (int i = 0; i < steps; i++) {
		pos = pos + step;
		
		vec3 projected = Project(vec4(pos, 1));
		vec2 uv = projected.xy * 0.5 + 0.5;

		// Out of bounds
		if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) break;

		//vec3 worldPosAtUV = getWorldPos(uv);
		//float depthDiff = projected.z - worldPosAtUV.z;

		float sample_depth = GetDepth(uv);
		float proj_depth = projected.z;
		float depthDiff = proj_depth - sample_depth;

		if (depthDiff >0.0 && depthDiff < 0.01) {
			// Hit
			return uv;
		}

		if (length(P - pos) > maxDist) break; // Miss (out of length)

	}

	return vec2(-1);
}


void main() {

	vec2 uv = o_position.xy * 0.5 + 0.5;
	
	vec4  u0 = texture(sampler2D(t_unit0, smplr), uv);
	vec4  u1 = texture(sampler2D(t_unit1, smplr), uv);
	vec4  u2 = texture(sampler2D(t_unit2, smplr), uv);
	float D  = texture(sampler2D(t_unit3, smplr), uv).r;

	vec3 C = u0.rgb; // Color
	vec3 N = normalize(u1.xyz); // Normal
	vec3 P = u2.xyz; // World position

	float metallic  = u0.a;
	float roughness = u1.a;
	float emissive  = u2.a;

	vec3 V = normalize(P - ubo.cam_pos.xyz);

	float fresnel = pow(1.0 - max(dot(V, N), 0.0), 5.0);
	float fresnel_metal = mix(0.04, 1.0, metallic);
	float fFactor = fresnel_metal + (1.0 - fresnel_metal) * fresnel;
	float intensity = fFactor * (1.0 - roughness) * 0.5;

	
	
	vec3 lColor = vec3(0);
	int rays = 2;
#if 0
	for (int i = 0; i < rays; i++) {
		vec3 reflected = normalize(reflect(V, N));

		vec3 rayDir = reflected;

		vec2 UV = TraceRay(P, rayDir);

		if (UV.x >= 0) {
			vec3 hit_color = texture(sampler2D(t_unit4, smplr), UV).rgb;
			lColor += hit_color * intensity;
		}
	}

	color = vec4(lColor / float(rays), 1.0);
#else
	color = vec4(vec3(0), 1.0);
#endif
}