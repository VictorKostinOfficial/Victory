#version 450

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;

void main() {
	gl_Position = vec4(inPos, 0.0, 1.0);
	outUV = inUV;
	outColor = inColor;
}