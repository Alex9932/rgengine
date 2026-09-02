#version 430 core

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 o_position;
layout(location = 1) out vec4 o_4pos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 camera;
	vec4 light_direction;
	vec4 light_color;
} push;

void main() {
    vec4 pos4 = push.mvp * vec4(position, 1.0);
    o_4pos = pos4;
    o_position  = pos4.xyz;
    gl_Position = pos4;
}