#version 430 core

layout(location = 0) in vec3 o_position;
layout(location = 1) in vec4 o_4pos;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;
layout(set = 1, binding = 1) uniform texture2D t_unit1;
layout(set = 1, binding = 2) uniform texture2D t_unit2;
layout(set = 1, binding = 3) uniform texture2D t_unit3; // depth

layout(push_constant) uniform PushConstants {
	mat4 mvp;
    vec4 camera;
	vec4 light_direction;
	vec4 light_color;
	vec4 light_position;
} push;

#define PI 3.14159265

float DistributionGGX(vec3 N, vec3 H, float r) {
	float a = r * r;
	float a2 = a*a;
	float NdotH = max(dot(N, H), 0);
	float NdotH2 = NdotH*NdotH;

	float nom = a2;
	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float r) {
	float R = (r + 1.0);
	float K = (R * R) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - K) + K;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float r) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx1 = GeometrySchlickGGX(NdotL, r);
	float ggx2 = GeometrySchlickGGX(NdotV, r);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float CalculateSpotlightCone(int type, vec3 L, vec3 D, float innerCone, float outerCone) {
	if (type == 1) { return 1.0; }
	float theta = dot(normalize(L), normalize(D));
	float intesity = smoothstep(outerCone, innerCone, theta);
	return intesity;
}

void main() {

	
    vec3 ndc = o_4pos.xyz / o_4pos.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;

	//vec2 uv = o_position.xy * 0.5 + 0.5;
	//vec2 uv = o_uv;// * 0.5 + 0.5;

	vec4 u0 = texture(sampler2D(t_unit0, smplr), uv);
	vec4 u1 = texture(sampler2D(t_unit1, smplr), uv);
	vec4 u2 = texture(sampler2D(t_unit2, smplr), uv);

	vec3 C = u0.rgb; // Color
	vec3 N = normalize(u1.xyz); // Normal
	vec3 P = u2.xyz; // World position

	float metallic  = u0.a;
	float roughness = u1.a;
	float emissive  = u2.a;

	vec3 V = normalize(push.camera.xyz - P);
	vec3 R = reflect(-V, N);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, C, metallic);

	vec3 L = normalize(push.light_position.xyz - P);
	vec3 H = normalize(V + L);

	float distance = length(push.light_position.xyz - P);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = push.light_color.xyz * push.light_direction.w * attenuation;

	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	float NdotL = max(dot(N, L), 0.0);

	vec3 c0 = (kD * C / PI + specular) * radiance * NdotL;
	vec3 c1 = C * max(0.0, emissive);

	//vec3 light = calculated_light[0] * calculated_shadow + calculated_light[1] + calculated_light[2];
	vec3 light = c0 + c1;

	light *= CalculateSpotlightCone(int(push.camera.w), L, push.light_direction.xyz, push.light_color.w, push.light_position.w);

	color = vec4(light, 1.0);

}