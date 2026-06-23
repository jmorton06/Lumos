#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#include "Buffers.glslh"
#include "Octahedral.glslh"

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) in VertexData VertexOutput;
// Packed G-buffer: rg = octahedral world normal, b = NDC depth, a = roughness.
layout(location = 0) out vec4 OutGBuffer;

float GetRoughness()
{
	if(u_MaterialData.RoughnessMapFactor < 0.05)
		return u_MaterialData.Roughness;
	return (1.0 - u_MaterialData.RoughnessMapFactor) * u_MaterialData.Roughness
	     + u_MaterialData.RoughnessMapFactor * texture(u_RoughnessMap, VertexOutput.TexCoord).r;
}

void main(void)
{
	vec2 octN = OctEncodeNormal(normalize(VertexOutput.Normal));
	OutGBuffer = vec4(octN, gl_FragCoord.z, GetRoughness());
}
