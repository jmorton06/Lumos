#version 450
#include "Common.glslh"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 colour;

layout (location = 0) in DATA
{
	vec3 position;
	vec4 colour;
	float edge;
} fs_in;

void main()
{
	vec4 c = DeGamma(fs_in.colour);

	float aa  = fwidth(fs_in.edge);
	float cov = 1.0 - smoothstep(1.0 - aa, 1.0, abs(fs_in.edge));
	c.a *= cov;

	colour = c;
}
