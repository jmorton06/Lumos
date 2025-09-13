#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"
#include "PBR.glslh"

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) in VertexData VertexOutput;

layout(location = 0) out vec4 OutNormal;
void main(void)
{
	float alpha = texture(u_AlbedoMap, VertexOutput.TexCoord).a;

	if(alpha < u_MaterialData.AlphaCutOff)
		discard;

	OutNormal = vec4(normalize(VertexOutput.Normal * 0.5 + 0.5), 1.0f);
}
