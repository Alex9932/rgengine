#version 430 core

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 o_position;

void main() {
	o_position  = position;
	gl_Position = vec4(position, 1.0);
}