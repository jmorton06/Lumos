#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"
#include "PBR.glslh"

layout(location = 0) in vec2 uv;

void main(void)
{
	float alpha = texture(u_AlbedoMap, uv).a;

	if(alpha < u_MaterialData.AlphaCutOff)
		discard;
}
