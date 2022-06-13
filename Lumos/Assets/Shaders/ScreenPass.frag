#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;


void main()
{
	vec3 colour = texture(u_Texture, outTexCoord).rgb;
	outFrag = vec4(colour, 1.0);
}