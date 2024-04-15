#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 colour;

layout (location = 0) in DATA
{
	vec3 position;
	vec4 colour;
} fs_in;

void main()
{
	colour = DeGamma(fs_in.colour);
}
