#version 430 core

layout(location = 0) in vec2 position;

layout(location = 0) out vec2 o_position;

void main() {
	o_position  = position;
	gl_Position = vec4(position, 0, 1.0);
}