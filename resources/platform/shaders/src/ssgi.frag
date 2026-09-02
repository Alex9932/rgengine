#version 430 core
#extension GL_EXT_control_flow_attributes : require

layout(location = 0) in vec2 o_position;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0; // color
layout(set = 1, binding = 1) uniform texture2D t_unit1; // normal
layout(set = 1, binding = 2) uniform texture2D t_unit2; // wpos
layout(set = 1, binding = 3) uniform texture2D t_unit3; // depth
layout(set = 2, binding = 0) uniform texture2D t_unit4; // calculated light

layout(push_constant) uniform PushConstants {
    vec4 camera;
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

const int rayCount = 4;
const int numSteps = 32;
const float rayStep = 0.05;
const float maxRayDistance = 5.0;
const float intensity = 1.0;

const vec3 noiseSamples[16] = vec3[](
    vec3( 0.5381,  0.1856, -0.4319),
    vec3( 0.1379,  0.2486,  0.4430),
    vec3( 0.3371,  0.5679, -0.0057),
    vec3(-0.6999, -0.0451, -0.0019),
    vec3( 0.0689, -0.1598, -0.8547),
    vec3( 0.0560,  0.0069,  0.1841),
    vec3(-0.0146,  0.1402,  0.0762),
    vec3( 0.0100, -0.1924, -0.0344),
    vec3(-0.3577, -0.5301, -0.4358),
    vec3(-0.3169,  0.1063,  0.0158),
    vec3( 0.0103, -0.5869,  0.0046),
    vec3(-0.0897, -0.4940,  0.3287),
    vec3( 0.7119, -0.0154,  0.0916),
    vec3(-0.0533, -0.0591,  0.5411),
    vec3( 0.0352, -0.0631, -0.5460),
    vec3(-0.4776,  0.2847, -0.0271)
);

float GetDepth(vec2 uv) {
	return texture(sampler2D(t_unit3, smplr), uv).r;
}

vec3 Project(vec4 P) {
	vec4 p = ubo.proj * ubo.view * P;
	return p.xyz / p.w;
}

// Return hit coord (or -1 if ray miss)
vec2 TraceRay(vec3 P, vec3 D) {
	
	float stepsize = 0.3;
	int   steps    = 300;

	vec3 step      = normalize(D) * stepsize;
	vec3 pos       = P;

	[[dont_unroll]]
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

		if (length(P - pos) > maxRayDistance) break; // Miss (out of length)

	}

	return vec2(-1);
}

vec3 cosineWeightedHemisphereDirection(vec3 normal, vec2 random) {
    // Метод Malley: проецируем равномерные точки с диска на полусферу
    float r = sqrt(random.x);
    float theta = 2.0 * PI * random.y;
    
    // Координаты на диске
    float x = r * cos(theta);
    float y = r * sin(theta);
    
    // Проекция на полусферу (z = sqrt(1 - x² - y²))
    float z = sqrt(max(0.0, 1.0 - x*x - y*y));
    
    // Построение ортонормированного базиса
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    
    return normalize(tangent * x + bitangent * y + normal * z);
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

	vec3 lColor = vec3(0);

	[[dont_unroll]]
	for(int i = 0; i < rayCount; i++) {
#if 0
		float phi = 2.0 * PI * float(i) / float(rayCount);
		float cosTheta = 1.0 - float(i) / float(rayCount);
		float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // sin^2 + cos^2 = 1 => sin = sqrt(1 - cos^2)
		vec3 rayDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
#else
		vec2 rnd = vec2(0);

		rnd.x = fract(sin(dot(uv + ubo.cpu_rnd.xy, vec2(12.9898, 78.233))) * 43758.5453);
		rnd.y = fract(sin(dot(uv + ubo.cpu_rnd.zw + vec2(1.0, 0.0), vec2(12.9898, 78.233))) * 43758.5453);

		vec3 rayDir = cosineWeightedHemisphereDirection(N, rnd);
		//vec3 rayDir = noiseSamples[i % 16];
		//vec3 V = normalize(P - ubo.cam_pos.xyz);
		//vec3 rayDir = normalize(reflect(V, N));
#endif

#if 0
		vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
		vec3 tangent = normalize(cross(up, N));
		vec3 bitangent = cross(N, tangent);
		mat3 TBN = mat3(tangent, bitangent, N);
		rayDir = normalize(TBN * rayDir);
#endif

		if (dot(rayDir, N) < 0) { rayDir = -rayDir; }

		vec2 UV = TraceRay(P, rayDir);

		if (UV.x < 0) { continue; } // Miss

		//lColor += texture(sampler2D(t_unit4, smplr), UV).rgb;

		// Use as light source
		vec3 hit_pos = texture(sampler2D(t_unit2, smplr), UV).xyz;
		vec3 hit_color = texture(sampler2D(t_unit4, smplr), UV).rgb;

		vec3 L = normalize(hit_pos - P);
		float radiance = max(0, dot(L, N));

		float distance = length(hit_pos - P);
		float atten = 1.0 / (distance * distance);

		lColor += hit_color * radiance;// * atten;

	}

	color = vec4(lColor / rayCount, 1.0);
}