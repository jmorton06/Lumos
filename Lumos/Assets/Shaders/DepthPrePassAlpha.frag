#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"
#include "PBR.glslh"
#include "Octahedral.glslh"
#define GBUFFER_MATERIAL
#include "GBuffer.glslh"

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) in VertexData VertexOutput;

// Packed G-buffer, see GBuffer.glslh
layout(location = 0) out vec4 OutGBuffer;

void main(void)
{
	float alpha = texture(u_AlbedoMap, VertexOutput.TexCoord).a;

	if(alpha < u_MaterialData.AlphaCutOff)
		discard;

	vec2 m    = GBufferMaterial(VertexOutput.TexCoord);
	vec2 octN = OctEncodeNormal(normalize(VertexOutput.Normal));
	OutGBuffer = vec4(octN, gl_FragCoord.z, GBufferPackMaterial(m.x, m.y));
}
