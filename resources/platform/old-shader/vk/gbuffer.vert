#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 o_position;
layout(location = 1) out vec3 o_T;
layout(location = 2) out vec3 o_B;
layout(location = 3) out vec3 o_N;
layout(location = 4) out vec2 o_uv;

layout(set = 0, binding = 0, std140) uniform Camera {
    mat4 proj;
	mat4 view;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
	mat4 mvp  = camera.proj * camera.view * push.model;
	mat3 md3  = mat3(push.model);
	vec4 pos4 = vec4(position, 1.0);

	vec3 N = md3 * normal;
	vec3 T = md3 * tangent;
	vec3 B = cross(N, T);

	o_position = (push.model * pos4).xyz;
	o_N        = normalize(N);
	o_T        = normalize(T);
	o_B        = normalize(B);
	o_uv       = uv;
	gl_Position = mvp * pos4;
}