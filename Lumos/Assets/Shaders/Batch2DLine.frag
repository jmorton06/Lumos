#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 color;

layout (location = 0) in DATA
{
	vec3 position;
	vec4 color;
} fs_in;

void main()
{
	color = fs_in.color;
}
